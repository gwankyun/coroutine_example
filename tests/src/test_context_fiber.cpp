module;

#include <boost/asio.hpp>
#include <boost/context/fiber.hpp>
#include <boost/scope/defer.hpp>
#include <spdlog.hpp>

module test.context.fiber;

import std;
import io_scheduler;
import spdlog;

using namespace std::chrono_literals;
namespace asio = boost::asio;
using asio::steady_timer;
using boost::system::error_code;
namespace ctx = boost::context;

struct Context
{
    std::unordered_map<int, std::string> result;
    std::queue<int> awake_cont;
    std::unordered_map<int, std::unique_ptr<ctx::fiber>> coro_cont;
    std::unordered_map<int, std::unique_ptr<steady_timer>> timer_cont;
};

void resume(ctx::fiber& _fiber)
{
    _fiber = std::move(_fiber).resume();
}

auto g_time = 100ms;

ctx::fiber make_fiber(Context& _context, int _id, steady_timer& _timer)
{
    auto fiber = [&, _id](ctx::fiber&& _sink) {
        auto& awake_cont = _context.awake_cont;
        auto& result = _context.result;

        // 協程結束時自動喚醒，確保協程能夠正常退出。
        auto awake = [&] { awake_cont.push(_id); };
        BOOST_SCOPE_DEFER[&]
        {
            awake();
        };

        error_code e;

        auto cb = [&](error_code& e) {
            return [&](error_code _e) {
                e = _e;
                awake();
            };
        };

        SPDLOG_INFO("");
        auto& timer = _timer;
        timer.expires_after(g_time);
        timer.async_wait(cb(e));
        // 協程暫停，等待異步操作完成後再繼續執行。
        resume(_sink);
        if (e)
        {
            SPDLOG_ERROR("{}", e.message());
            return std::move(_sink);
        }
        result[_id] = std::to_string(_id);

        SPDLOG_INFO("");
        // 支持循環內的異步操作，協程可以在循環內多次暫停和繼續執行，直到循環結束。
        for (int i = 0; i < 3; ++i)
        {
            timer.expires_after(g_time);
            timer.async_wait(cb(e));
            resume(_sink);
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return std::move(_sink);
            }
            result[_id] += "r";
        }

        SPDLOG_INFO("");
        timer.expires_after(g_time);
        timer.async_wait(cb(e));
        resume(_sink);
        if (e)
        {
            SPDLOG_ERROR("{}", e.message());
            return std::move(_sink);
        }
        result[_id] += "w";

        asio::post(timer.get_executor(), [&, _id] { _context.timer_cont.erase(_id); });

        return std::move(_sink);
    };
    return fiber;
};

std::unordered_map<int, std::string> test_context_fiber()
{
    asio::io_context io_ctx;
    Context context;

    auto& result = context.result;

    // awake_cont用於存儲需要喚醒的協程ID和對應的錯誤碼，當異步操作完成時，將結果傳回協程並喚醒協程繼續執行。
    auto& awake_cont = context.awake_cont;
    // coro_cont用於存儲協程ID和對應的協程對象，當需要喚醒協程時，從coro_cont中找到對應的協程對象並傳回異步操作的結果。
    auto& coro_cont = context.coro_cont;

    auto make_accept = [&](steady_timer& _acceptor) {
        auto fiber = [&](ctx::fiber&& _sink) {
            // 協程結束時自動喚醒，確保協程能夠正常退出。
            auto awake = [&] { awake_cont.push(0); };
            BOOST_SCOPE_DEFER[&]
            {
                awake();
            };

            error_code e;

            auto cb = [&](error_code& e) {
                return [&](error_code _e) {
                    e = _e;
                    awake();
                };
            };

            SPDLOG_INFO("");
            auto& acceptor = _acceptor;

            for (int id = 1; id < 4; ++id)
            {
                acceptor.expires_after(g_time);
                acceptor.async_wait(cb(e));
                resume(_sink);
                if (e)
                {
                    SPDLOG_ERROR("{}", e.message());
                    return std::move(_sink);
                }

                context.timer_cont[id] = std::make_unique<steady_timer>(acceptor.get_executor());
                auto& timer = *context.timer_cont[id];
                coro_cont[id] = std::make_unique<ctx::fiber>(make_fiber(context, id, timer));

                auto& fiber = *coro_cont[id];
                // 惰性協程，創建後需要手動運行
                resume(fiber);
            }

            return std::move(_sink);
        };
        return fiber;
    };

    auto main_fiber = [&] {
        steady_timer acceptor(io_ctx);
        coro_cont[0] = std::make_unique<ctx::fiber>(make_accept(acceptor));
        // 惰性協程，創建後需要手動運行
        resume(*coro_cont[0]);

        run_scheduler<ctx::fiber>(io_ctx, coro_cont, awake_cont, [](auto& _fiber) { resume(_fiber); });
    };

    auto main = std::make_unique<ctx::fiber>([&](ctx::fiber&& _sink) {
        SPDLOG_INFO("enter main");
        main_fiber();
        return std::move(_sink);
    });
    resume(*main);

    return result;
}
