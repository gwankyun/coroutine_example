module;
#include "use_module.h"

#if !USE_STD_MODULE
#  include <atomic>
#  include <chrono>
#  include <functional>
#  include <mutex>
#  include <queue>
#  include <string>
#  include <thread>
#  include <tuple>
#  include <vector>
#endif

#include "catch2.h"

#include "spdlog.h"

#include <asio.hpp>

#include "task.hpp"

export module asio_strand;

#if USE_STD_MODULE
import std;
#endif

#if USE_THIRD_MODULE
import catch2.compat;
import spdlog;
#endif

using namespace std::literals::chrono_literals;

std::mutex g_mutex;

std::queue<std::function<void()>> g_task_queue;

std::atomic<bool> flag(true);

type::awaiter async_post_resume(type::coroutine& _coroutine, asio::io_context& _io_context, std::function<void()> _f)
{
    return func::async_resume(_coroutine, [&, _f] { asio::post(_io_context, _f); });
}

type::awaiter async_post(type::coroutine& _coroutine, asio::io_context& _io_context, std::function<void()> _f)
{
    auto cb = [&, _f]
    {
        _f();
        _coroutine.resume();
    };
    return func::async_resume(_coroutine, [&, cb] { asio::post(_io_context, cb); });
}

type::task run(asio::io_context& _io_context, std::string _name)
{
    type::coroutine coroutine; // 用於保存協程句柄
    auto resume = [&] { coroutine.resume(); };

    for (auto i = 0; i != 3; ++i)
    {
        co_await async_post(coroutine, _io_context, [&] { SPDLOG_INFO("name: {} i: {}", _name, i); });
        {
            std::lock_guard<std::mutex> guard(g_mutex);
            g_task_queue.push(
                [&]
                {
                    std::this_thread::sleep_for(100ms);
                    SPDLOG_INFO("name: {} i: {} sleep", _name, i);
                    coroutine.resume();
                }
            );
        }
        co_await async_post_resume(coroutine, _io_context, [] {});
    }

    co_return;
}

type::task run_all(asio::io_context& _io_context)
{
    type::coroutine coroutine; // 用於保存協程句柄

    for (auto i = 0; i != 10; ++i)
    {
        run(_io_context, std::to_string(i));
    }

    flag = false;

    co_return;
}

export int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [t:%6t] [%-10!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    asio::io_context::strand strand_1(io_context);
    asio::io_context::strand strand_2(io_context);

    auto concurrency = std::thread::hardware_concurrency();

    std::jthread task_thread(
        [&]
        {
            while (true)
            {
                SPDLOG_INFO("");
                std::this_thread::sleep_for(100ms);
                {
                    std::lock_guard<std::mutex> guard(g_mutex);
                    if (!g_task_queue.empty())
                    {
                        SPDLOG_INFO("");
                        auto fn = g_task_queue.front();
                        g_task_queue.pop();
                        fn();
                        // coro->resume();
                    }
                    else
                    {
                        // std::this_thread::sleep_for(1000ms);
                        continue;
                    }
                }
                // if (!flag.load())
                // {
                //     SPDLOG_INFO("");
                //     return;
                // }
            }
        }
    );

    run_all(io_context);

    std::vector<std::jthread> thread_cont;

    for (auto i = 0u; i != concurrency; ++i)
    {
        thread_cont.emplace_back([&] { io_context.run(); });
    }

    return 0;
}
