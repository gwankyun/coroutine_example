module;

#include <boost/asio.hpp>
#include <boost/context/continuation.hpp>
#include <boost/scope/defer.hpp>
#include <spdlog/spdlog.h>

module test.continuation;

import std;

using namespace std::chrono_literals;
using boost::asio::steady_timer;
using boost::system::error_code;

std::string test_continuation()
{
    boost::asio::io_context context;

    std::string result = "0";

    auto time = 100ms;

    // awake_cont用於存儲需要喚醒的協程ID和對應的錯誤碼，當異步操作完成時，將結果傳回協程並喚醒協程繼續執行。
    std::queue<int> awake_cont;
    namespace ctx = boost::context;
    // coro_cont用於存儲協程ID和對應的協程對象，當需要喚醒協程時，從coro_cont中找到對應的協程對象並傳回異步操作的結果。
    std::unordered_map<int, std::unique_ptr<ctx::continuation>> coro_cont;

    auto make_continuation = [&](int _id)
    {
        auto continuation = [&, _id](ctx::continuation&& _sink)
        {
            // 協程結束時自動喚醒，確保協程能夠正常退出。
            auto awake = [&] { awake_cont.push(_id); };
            BOOST_SCOPE_DEFER[&]
            {
                awake();
            };

            error_code e;

            auto cb = [&](std::string _message)
            {
                return [&, _message](error_code _e)
                {
                    result += _message;
                    e = _e;
                    awake();
                };
            };

            SPDLOG_INFO("");
            steady_timer accept(context, time);
            accept.async_wait(cb("1"));
            // 協程暫停，等待異步操作完成後再繼續執行。
            _sink = _sink.resume();
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                // return;
                return std::move(_sink);
            }

            SPDLOG_INFO("");
            // 支持循環內的異步操作，協程可以在循環內多次暫停和繼續執行，直到循環結束。
            for (int i = 0; i < 3; ++i)
            {
                steady_timer read(context, time);
                read.async_wait(cb("2"));
                _sink = _sink.resume();
                if (e)
                {
                    SPDLOG_ERROR("{}", e.message());
                    return std::move(_sink);
                }
            }

            SPDLOG_INFO("");
            steady_timer write(context, time);
            write.async_wait(cb("3"));
            _sink = _sink.resume();
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return std::move(_sink);
            }
            return std::move(_sink);
        };
        return ctx::callcc(continuation);
    };

    auto main_continuation = [&]
    {
        for (int id = 0; id < 1; ++id)
        {
            coro_cont[id] = std::make_unique<ctx::continuation>(make_continuation(id));
        }

        // 調度算法：每次運行io_context後，檢查是否有協程需要喚醒，如果有則喚醒一個協程，直到所有協程都完成。
        while (!coro_cont.empty())
        {
            context.run();
            if (!awake_cont.empty())
            {
                auto id = awake_cont.front();
                awake_cont.pop();

                auto iter = coro_cont.find(id);
                if (iter != coro_cont.end())
                {
                    auto& continuation = *iter->second;
                    // 判斷協程是否已經完成，完成就移除
                    if (!continuation)
                    {
                        coro_cont.erase(id);
                        continue;
                    }
                    // 將異步操作的結果傳回協程，讓協程繼續執行
                    continuation = continuation.resume();
                }
            }
        }
    };

    auto main = std::make_unique<ctx::continuation>(ctx::callcc(
        [&](ctx::continuation&& _sink)
        {
            SPDLOG_INFO("enter main");
            main_continuation();
            return std::move(_sink);
        }));

    return result;
}
