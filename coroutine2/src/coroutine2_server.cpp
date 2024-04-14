#include "log.hpp"
#include <spdlog/fmt/bin_to_hex.h>
#include "connection.h"
#define BOOST_ALL_NO_LIB 1
#include <boost/coroutine2/all.hpp>
#include <format>
#include <map>
#include <set>

namespace coro2 = boost::coroutines2;

template<typename T = void>
using pull_type = typename coro2::coroutine<T>::pull_type;

template<typename T = void>
using push_type = typename coro2::coroutine<T>::push_type;

inline std::string get_info(const socket_t& socket)
{
    auto remote_endpoint = socket.remote_endpoint();
    auto ip = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    return std::format("({} : {})", ip, port);
}

void coro_example(asio::io_context& _io_context, acceptor_t& _acceptor)
{
    std::map<int, std::unique_ptr<pull_type<bool>>> child;
    int id = 0;

    std::set<int> awake_set;

    auto cb = [&awake_set](int _id, error_code_t& _error, std::size_t& _bytes)
        {
            return [&awake_set, _id, &_error, &_bytes](error_code_t _e, std::size_t _b = 0u)
                {
                    _error = _e;
                    _bytes = _b;
                    awake_set.insert(_id);
                };
        };

    child[id] = std::make_unique<pull_type<bool>>(
        [&id, cb, &_acceptor, &child](push_type<bool>& _yield)
        {
            int current = 0;
            while (true)
            {
                error_code_t error;
                std::size_t bytes;
                socket_t socket(_acceptor.get_executor());
                _acceptor.async_accept(socket, cb(current, error, bytes));
                _yield(true);

                if (error)
                {
                    SPDLOG_WARN("{} _error: {}", get_info(socket), error.message());
                    continue;
                }

                SPDLOG_INFO("{} new", get_info(socket));

                id++;
                auto newID = id;
                child[newID] = std::make_unique<pull_type<bool>>(
                    [newID, cb, socket = std::move(socket), &child](push_type<bool>& _yield) mutable
                    {
                        int current = newID;
                        std::vector<unsigned char> buffer;
                        std::size_t offset = 0u;

                        error_code_t error;
                        std::size_t bytes;

                        while (offset == 0 || buffer[offset - 1] != '\0')
                        {
                            buffer.resize(offset + 4, 0xFF);
                            socket.async_read_some(
                                asio::buffer(buffer) += offset,
                                cb(current, error, bytes));
                            _yield(true);

                            if (error)
                            {
                                SPDLOG_WARN("{} _error: {}", get_info(socket), error.message());
                                _yield(false);
                            }

                            if (bytes > 0)
                            {
                                SPDLOG_INFO("{} bytes: {}", get_info(socket), bytes);
                                offset += bytes;
                            }
                        }

                        SPDLOG_INFO("{} id: {} {}",
                            get_info(socket),
                            current,
                            reinterpret_cast<const char*>(buffer.data()));

                        buffer.resize(offset);
                        offset = 0;
                        while (offset < buffer.size())
                        {
                            socket.async_write_some(
                                asio::buffer(buffer) += offset,
                                cb(current, error, bytes));
                            _yield(true);

                            if (error)
                            {
                                SPDLOG_WARN("{} _error: {}", get_info(socket), error.message());
                                _yield(false);
                            }

                            if (bytes > 0)
                            {
                                SPDLOG_INFO("{} bytes: {}", get_info(socket), bytes);
                                offset += bytes;
                            }
                        }

                        SPDLOG_INFO("{} id: {} write finished", get_info(socket), current);
                        _yield(false);
                    });
            }
        });

    while (!child.empty())
    {
        std::vector<int> key_vec;
        for (auto& i : child)
        {
            key_vec.push_back(i.first);
        }

        for (auto& i : key_vec)
        {
            if (awake_set.count(i) > 0)
            {
                auto& resume = *(child[i]);
                if (!resume || !resume().get())
                {
                    child.erase(i);
                    SPDLOG_INFO("child size: {}", child.size());
                }
                awake_set.erase(i);
            }
        }
        _io_context.run();
    }
}

int main(int _argc, char* _argv[])
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v");

    Argument argument;
    argument.parse_server(_argc, _argv);

    asio::io_context io_context;
    using asio::ip::tcp;

    acceptor_t acceptor(
        io_context,
        tcp::endpoint(tcp::v4(), argument.port));

    coro_example(io_context, acceptor);

    return 0;
}
