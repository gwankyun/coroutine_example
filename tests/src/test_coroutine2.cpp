module;

#include <boost/asio.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/scope/defer.hpp>
#include <spdlog/spdlog.h>

module test.coroutine2;

import std;
import io_scheduler;

using namespace std::chrono_literals;
using boost::asio::steady_timer;
using boost::system::error_code;
namespace coro2 = boost::coroutines2;
using coro_type = coro2::coroutine<bool>;

struct Context
{
    std::unordered_map<int, std::string> result;
    /// awake_cont用於存儲需要喚醒的協程ID，當異步操作完成時，喚醒協程繼續執行。
    std::queue<int> awake_cont;
    /// coro_cont用於存儲協程ID和對應的協程對象，當需要喚醒協程時，從coro_cont中找到對應的協程對象。
    std::unordered_map<int, std::unique_ptr<coro_type::pull_type>> coro_cont;
};

typename coro_type::pull_type make_coro(Context& _context, int _id, std::shared_ptr<steady_timer> _timer)
{
    typename coro_type::pull_type pull([&, _id, _timer](typename coro_type::push_type& _push) {
        auto& awake_cont = _context.awake_cont;
        auto& result = _context.result;
        auto time = 100ms;

        // 協程結束時自動喚醒，確保協程能夠正常退出。
        auto awake = [&] { awake_cont.push(_id); };
        BOOST_SCOPE_DEFER[&]
        {
            awake();
        };

        auto cb = [&](error_code& e) {
            return [&](error_code _e) {
                e = _e;
                awake();
            };
        };

        error_code e;

        SPDLOG_INFO("");
        auto& timer = *_timer;
        timer.expires_after(time);
        timer.async_wait(cb(e));
        // 協程暫停，等待異步操作完成後再繼續執行。
        _push(false);
        // 獲取異步操作的結果，如果有錯誤則輸出錯誤信息並退出協程。
        if (e)
        {
            SPDLOG_ERROR("{}", e.message());
            return;
        }
        result[_id] = std::to_string(_id);

        SPDLOG_INFO("");
        // 支持循環內的異步操作，協程可以在循環內多次暫停和繼續執行，直到循環結束。
        for (int i = 0; i < 3; ++i)
        {
            timer.expires_after(time);
            timer.async_wait(cb(e));
            _push(false);
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return;
            }
            result[_id] += "r";
        }

        SPDLOG_INFO("");
        timer.expires_after(time);
        timer.async_wait(cb(e));
        _push(false);
        if (e)
        {
            SPDLOG_ERROR("{}", e.message());
            return;
        }
        result[_id] += "w";
        _push(true);
    });
    return pull;
};

std::unordered_map<int, std::string> test_coroutine2()
{
    Context context;

    auto& result = context.result;

    auto time = 100ms;

    auto& awake_cont = context.awake_cont;
    auto& coro_cont = context.coro_cont;

    auto make_accept = [&](std::shared_ptr<steady_timer> _acceptor) {
        return [&, _acceptor](typename coro_type::push_type& _push) {
            // 協程結束時自動喚醒，確保協程能夠正常退出。
            auto awake = [&] { awake_cont.push(0); };
            BOOST_SCOPE_DEFER[&]
            {
                awake();
            };

            auto cb = [&](error_code& e) {
                return [&](error_code _e) {
                    e = _e;
                    awake();
                };
            };

            error_code e;

            SPDLOG_INFO("");
            auto& acceptor = *_acceptor;

            for (int id = 1; id < 4; ++id)
            {
                acceptor.expires_after(time);
                acceptor.async_wait(cb(e));
                _push(false);

                if (e)
                {
                    SPDLOG_ERROR("{}", e.message());
                    return;
                }

                auto timer = std::make_shared<steady_timer>(acceptor.get_executor());
                coro_cont[id] = std::make_unique<coro_type::pull_type>(make_coro(context, id, timer));
            }

            _push(true);
        };
    };

    boost::asio::io_context io_ctx;

    {
        auto timer = std::make_shared<steady_timer>(io_ctx);
        coro_cont[0] = std::make_unique<coro_type::pull_type>(make_accept(timer));
        // 急切協程，創建後自動運行
    }

    run_scheduler<coro_type::pull_type>(io_ctx, coro_cont, awake_cont, [](auto& pull) {
        pull();
        // 演示數據傳遞
        auto last = pull.get();
        SPDLOG_INFO("last: {}", last);
        if (last) // 如果是最後一行需要再喚醒一次
        {
            pull();
        }
    });
    return result;
}
