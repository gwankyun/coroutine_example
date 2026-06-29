module;

#include <boost/asio.hpp>
#include <boost/context/fiber.hpp>
#include <boost/scope/defer.hpp>
#include <spdlog/spdlog.h>

module test.context.fiber;

import std;
import io_scheduler;

using namespace std::chrono_literals;
using boost::asio::steady_timer;
using boost::system::error_code;
namespace ctx = boost::context;

struct Context
{
    std::unordered_map<int, std::string> result;
    std::queue<int> awake_cont;
    std::unordered_map<int, std::unique_ptr<ctx::fiber>> coro_cont;
};

ctx::fiber make_fiber(Context& _context, int _id, std::shared_ptr<steady_timer> _timer)
{
    auto fiber = [&, _id, _timer](ctx::fiber&& _sink) {
        auto& awake_cont = _context.awake_cont;
        auto& result = _context.result;
        auto time = 100ms;

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
        auto& timer = *_timer;
        timer.expires_after(time);
        timer.async_wait(cb(e));
        // 協程暫停，等待異步操作完成後再繼續執行。
        _sink = std::move(_sink).resume();
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
            timer.expires_after(time);
            timer.async_wait(cb(e));
            _sink = std::move(_sink).resume();
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return std::move(_sink);
            }
            result[_id] += "r";
        }

        SPDLOG_INFO("");
        timer.expires_after(time);
        timer.async_wait(cb(e));
        _sink = std::move(_sink).resume();
        if (e)
        {
            SPDLOG_ERROR("{}", e.message());
            return std::move(_sink);
        }
        result[_id] += "w";
        return std::move(_sink);
    };
    return fiber;
};

std::unordered_map<int, std::string> test_context_fiber()
{
    Context context;

    auto& result = context.result;

    auto time = 100ms;

    // awake_cont用於存儲需要喚醒的協程ID和對應的錯誤碼，當異步操作完成時，將結果傳回協程並喚醒協程繼續執行。
    auto& awake_cont = context.awake_cont;
    // coro_cont用於存儲協程ID和對應的協程對象，當需要喚醒協程時，從coro_cont中找到對應的協程對象並傳回異步操作的結果。
    auto& coro_cont = context.coro_cont;

    auto make_accept = [&](std::shared_ptr<steady_timer> _timer) {
        auto fiber = [&, _timer](ctx::fiber&& _sink) {
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
            auto& acceptor = *_timer;

            for (int id = 1; id < 4; ++id)
            {
                acceptor.expires_after(100ms);
                acceptor.async_wait(cb(e));
                _sink = std::move(_sink).resume();
                if (e)
                {
                    SPDLOG_ERROR("{}", e.message());
                    return std::move(_sink);
                }
                auto timer = std::make_shared<steady_timer>(acceptor.get_executor());
                coro_cont[id] = std::make_unique<ctx::fiber>(make_fiber(context, id, timer));
                auto& fiber = *coro_cont[id];
                fiber = std::move(fiber).resume();
            }

            return std::move(_sink);
        };
        return fiber;
    };

    boost::asio::io_context io_ctx;

    auto main_fiber = [&] {
        {
            auto acceptor = std::make_shared<steady_timer>(io_ctx);
            coro_cont[0] = std::make_unique<ctx::fiber>(make_accept(acceptor));
            auto& acceptor_fiber = *coro_cont[0];
            acceptor_fiber = std::move(acceptor_fiber).resume();
        }

        run_scheduler<ctx::fiber>(io_ctx, coro_cont, awake_cont,
                                  [](auto& fiber) { fiber = std::move(fiber).resume(); });
    };

    auto main = std::make_unique<ctx::fiber>([&](ctx::fiber&& _sink) {
        SPDLOG_INFO("enter main");
        main_fiber();
        return std::move(_sink);
    });
    *main = std::move(*main).resume();

    return result;
}
