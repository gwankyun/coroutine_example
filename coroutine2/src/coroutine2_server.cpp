﻿#include "connection.h"
// #include "log.hpp"
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>
#define BOOST_ALL_NO_LIB 1
#include "json.hpp"
#include <boost/coroutine2/all.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <queue>
#include <unordered_map>
namespace fs = std::filesystem;

struct Global
{
    const char end_char = '\0';
    const unsigned char empty_char = 0xFF;
    int wait_seconds = 3;
} global;

namespace coro2 = boost::coroutines2;

template <typename T = void>
using pull_type = typename coro2::coroutine<T>::pull_type;

template <typename T = void>
using push_type = typename coro2::coroutine<T>::push_type;

std::string get_info(const socket_t& socket)
{
    auto remote_endpoint = socket.remote_endpoint();
    auto ip = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    return std::format("({} : {})", ip, port);
}

struct OnExit
{
    using fn_t = std::function<void()>;
    OnExit(fn_t _fn) : fn(_fn)
    {
    }
    ~OnExit()
    {
        fn();
    }
    fn_t fn;
};

using steady_timer_t = asio::steady_timer;

void coro_example(asio::io_context& _io_context, acceptor_t& _acceptor)
{
    using pull_t = pull_type<>;
    using push_t = push_type<>;

    // 協程容器
    std::unordered_map<int, std::unique_ptr<pull_t>> coro_cont;

    // 待喚醒協程隊列。
    std::queue<int> awake_cont;

    // 通用回調，標記哪些協程需要被喚醒。
    auto callback = [&awake_cont](int _id, error_code_t& _error, std::size_t& _bytes)
    {
        return [&awake_cont, _id, &_error, &_bytes](error_code_t _e, std::size_t _b = 0u)
        {
            _error = _e;
            _bytes = _b;
            awake_cont.push(_id);
        };
    };

    // 每個連接一個新的函數，協程間不能共用函數，會有棧衝突
    auto connection_coro = [callback](int newID, socket_t&& socket)
    {
        return [newID, socket = std::move(socket), callback](push_t& _yield) mutable
        {
            int current = newID;
            std::vector<unsigned char> buffer;
            std::size_t offset = 0u;

            error_code_t error;
            std::size_t bytes;

            // 回調再封裝一下
            auto cb = callback(current, error, bytes);

            // 協程退出時也標記一下好統一處理
            OnExit onExit([&cb, &error, &bytes] { cb(error, bytes); });

            while (offset == 0 || buffer[offset - 1] != global.end_char)
            {
                buffer.resize(offset + 4, global.empty_char);
                auto buf = asio::buffer(buffer) += offset;
                socket.async_read_some(buf, cb);
                _yield();

                if (error)
                {
                    SPDLOG_WARN("{} _error: {}", get_info(socket), error.message());
                    return;
                }

                if (bytes > 0)
                {
                    SPDLOG_INFO("{} bytes: {}", get_info(socket), bytes);
                    offset += bytes;
                }
            }

            auto buf = reinterpret_cast<const char*>(buffer.data());
            SPDLOG_INFO("{} id: {} {}", get_info(socket), current, buf);

            asio::chrono::seconds t(global.wait_seconds);
            steady_timer_t timer(socket.get_executor(), t);
            timer.async_wait(cb);
            _yield();
            SPDLOG_INFO("{} timeout!", get_info(socket));

            buffer.resize(offset);
            offset = 0;
            while (offset < buffer.size())
            {
                auto buf = asio::buffer(buffer) += offset;
                socket.async_write_some(buf, cb);
                _yield();

                if (error)
                {
                    SPDLOG_WARN("{} _error: {}", get_info(socket), error.message());
                    return;
                }

                if (bytes > 0)
                {
                    SPDLOG_INFO("{} bytes: {}", get_info(socket), bytes);
                    offset += bytes;
                }
            }

            SPDLOG_INFO("{} id: {} write finished", get_info(socket), current);
        };
    };

    // 監聽協程
    auto listen_id = 0;
    coro_cont[listen_id] = std::make_unique<pull_t>(
        [&listen_id, &callback, &_acceptor, &coro_cont, &connection_coro](push_t& _yield)
        {
            int current = listen_id;
            // 自增協程標識
            int connection_id = current + 1;

            error_code_t error;
            std::size_t bytes;

            auto cb = callback(current, error, bytes);

            OnExit onExit([&cb, &error, &bytes] { cb(error, bytes); });

            while (true)
            {
                socket_t socket(_acceptor.get_executor());
                _acceptor.async_accept(socket, cb);
                _yield();

                if (error)
                {
                    SPDLOG_WARN("{} _error: {}", get_info(socket), error.message());
                    continue;
                }

                SPDLOG_INFO("{} new", get_info(socket));

                coro_cont[connection_id] = std::make_unique<pull_t>(connection_coro(connection_id, std::move(socket)));
                connection_id++;
            }
        });

    // 監聽協程永遠不會中止，所以會一直執行
    while (!coro_cont.empty())
    {
        // 實際執行異步操作
        _io_context.run();

        if (!awake_cont.empty())
        {
            SPDLOG_INFO("awake_cont size: {}", awake_cont.size());
        }

        // 簡單的協程調度，按awake_cont中先進先出的順序喚醒協程。
        while (!awake_cont.empty())
        {
            auto i = awake_cont.front();
            awake_cont.pop();
            SPDLOG_INFO("awake id: {}", i);
            auto iter = coro_cont.find(i);
            if (iter != coro_cont.end())
            {
                auto& resume = *(iter->second);
                if (!resume)
                {
                    coro_cont.erase(iter);
                    SPDLOG_INFO("child size: {}", coro_cont.size());
                    continue;
                }
                resume();
            }
        }
    }
}

int main(int _argc, char* _argv[])
{
    (void)_argc;
    (void)_argv;

    auto config_file = fs::current_path().parent_path() / "config.json";

    auto config = config::from_path(config_file);

    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    config::get(config, "log", config::is_string, log_format);
    spdlog::set_pattern(log_format);

    std::uint16_t port = 8888;
    config::get(config, "port", config::is_number, port);

    asio::io_context io_context;
    using asio::ip::tcp;

    acceptor_t acceptor(io_context, tcp::endpoint(tcp::v4(), port));

    coro_example(io_context, acceptor);

    return 0;
}
