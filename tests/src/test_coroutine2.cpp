module;

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <boost/coroutine2/all.hpp>
#include <boost/scope/defer.hpp>

module test.coroutine2;

import std;

using namespace std::chrono_literals;
using boost::asio::steady_timer;
using boost::system::error_code;

std::string test_coroutine2()
{
    boost::asio::io_context context;

    std::string result = "0";

    auto time = 100ms;

    // awake_cont用於存儲需要喚醒的協程ID和對應的錯誤碼，當異步操作完成時，將結果傳回協程並喚醒協程繼續執行。
    std::queue<std::tuple<int, error_code>> awake_cont;
    using coro_type = boost::coroutines2::coroutine<error_code>;
    // coro_cont用於存儲協程ID和對應的協程對象，當需要喚醒協程時，從coro_cont中找到對應的協程對象並傳回異步操作的結果。
    std::unordered_map<int, std::unique_ptr<coro_type::push_type>> coro_cont;

    auto make_coro = [&](int _id)
    {
        return [&, _id](typename coro_type::pull_type& _push)
        {
            // 協程結束時自動喚醒，確保協程能夠正常退出。
            auto awake = [&] { awake_cont.push(std::make_tuple(_id, error_code())); };
            BOOST_SCOPE_DEFER[&]
            {
                awake();
            };

            auto cb = [&](std::string _message)
            {
                return [&, _message](error_code)
                {
                    result += _message;
                    awake();
                };
            };

            error_code e;

            SPDLOG_INFO("");
            steady_timer accept(context, time);
            accept.async_wait(cb("1"));
            // 協程暫停，等待異步操作完成後再繼續執行。
            _push();
            // 獲取異步操作的結果，如果有錯誤則輸出錯誤信息並退出協程。
            e = _push.get();
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return;
            }

            SPDLOG_INFO("");
            // 支持循環內的異步操作，協程可以在循環內多次暫停和繼續執行，直到循環結束。
            for (int i = 0; i < 3; ++i)
            {
                steady_timer read(context, time);
                read.async_wait(cb("2"));
                _push();
                e = _push.get();
                if (e)
                {
                    SPDLOG_ERROR("{}", e.message());
                    return;
                }
            }

            SPDLOG_INFO("");
            steady_timer write(context, time);
            write.async_wait(cb("3"));
            _push();
            e = _push.get();
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return;
            }
        };
    };

    for (int id = 0; id < 1; ++id)
    {
        coro_cont[id] = std::make_unique<coro_type::push_type>(make_coro(id));
        // push_type創建後不會自動運行，需要手動啟動
        (*coro_cont[id])(error_code());
    }

    // 調度算法：每次運行io_context後，檢查是否有協程需要喚醒，如果有則喚醒一個協程，直到所有協程都完成。
    while (!coro_cont.empty())
    {
        context.run();
        if (!awake_cont.empty())
        {
            auto [id, err] = awake_cont.front();
            awake_cont.pop();

            auto iter = coro_cont.find(id);
            if (iter != coro_cont.end())
            {
                auto& pull = *iter->second;
                // 判斷協程是否已經完成，完成就移除
                if (!pull)
                {
                    coro_cont.erase(id);
                    continue;
                }
                // 將異步操作的結果傳回協程，讓協程繼續執行
                pull(err);
            }
        }
    }
    return result;;
}
