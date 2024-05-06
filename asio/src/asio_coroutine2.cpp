#include <boost/asio.hpp>
#include <boost/system.hpp> // boost::system::error_code
#include <spdlog/spdlog.h>
#include <string>
#define BOOST_ALL_NO_LIB 1
#include <boost/coroutine2/all.hpp>
#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>

namespace asio = boost::asio;
using error_code_t = boost::system::error_code;

namespace coro2 = boost::coroutines2;

template <typename T = void>
using pull_type = typename coro2::coroutine<T>::pull_type;

template <typename T = void>
using push_type = typename coro2::coroutine<T>::push_type;

void handle_old(asio::io_context& _io_context)
{
    using pull_t = pull_type<>;
    using push_t = push_type<>;

    bool flag = false;

    pull_t resume(
        [&flag, &_io_context](push_t& _yield)
        {
            error_code_t error;
            auto cb = [&flag, &error](error_code_t _e = error_code_t{})
            {
                error = _e;
                flag = true;
            };

            asio::post(_io_context, cb);
            flag = false;
            _yield();

            SPDLOG_INFO("read");
            asio::post(_io_context, cb);
            flag = false;
            _yield();

            SPDLOG_INFO("write");
            asio::steady_timer timer(_io_context, asio::chrono::seconds(1));
            timer.async_wait(cb);
            flag = false;
            _yield();

            if (error)
            {
                SPDLOG_INFO("{}", error.message());
                return;
            }
            SPDLOG_INFO("timeout");
        });

    while (resume)
    {
        _io_context.run();
        if (flag)
        {
            resume();
        }
    }
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

using pull_t = pull_type<>;
using push_t = push_type<>;

void handle(asio::io_context& _io_context)
{
    // 協程容器
    std::unordered_map<int, std::unique_ptr<pull_t>> coro_cont;

    // 待喚醒協程隊列。
    std::queue<int> awake_cont;

    auto create_coro = [&_io_context, &awake_cont](int id)
    {
        return [&_io_context, &awake_cont, id](push_t& _yield)
        {
            auto cb = [&awake_cont, id] { awake_cont.push(id); };
            OnExit onExit(cb);
            std::vector<int> data{1, 2, 3};
            for (auto& i : data)
            {
                SPDLOG_INFO("id: {} value: {}", id, i);
                asio::post(_io_context, cb);
                _yield();
            }
        };
    };

    for (auto id = 0; id < 3; id++)
    {
        coro_cont[id] = std::make_unique<pull_t>(create_coro(id));
    }

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

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    // handle_old(io_context);
    handle(io_context);

    return 0;
}
