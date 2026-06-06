module;

#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>
#include <boost/fiber/algo/round_robin.hpp>
#include <boost/scope/defer.hpp>
#include <spdlog/spdlog.h>

module test.fiber;

import std;

using namespace std::chrono_literals;
namespace asio = boost::asio;
using asio::steady_timer;
using boost::system::error_code;
namespace fibers = boost::fibers;

auto async_wait(steady_timer & timer) -> error_code
{
    std::promise<error_code> promise;
    auto future = promise.get_future();
    timer.async_wait(
        [promise = std::move(promise)](error_code e) mutable
        {
            promise.set_value(e);
        });
    return future.get();
}

std::string test_fiber()
{
    asio::io_context context;
    auto work_guard = asio::make_work_guard(context);
    std::thread io_thread([&]{ context.run(); });

    fibers::use_scheduling_algorithm<fibers::algo::round_robin>();

    std::string result = "0";
    auto time = 100ms;

    fibers::fiber fiber([&]
    {
        {
            steady_timer accept(context, time);
            auto e = async_wait(accept);
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return;
            }
            result += "1";
        }

        for (int i = 0; i < 3; ++i)
        {
            steady_timer read(context, time);
            auto e = async_wait(read);
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return;
            }
            result += "2";
        }

        {
            steady_timer write(context, time);
            auto e = async_wait(write);
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return;
            }
            result += "3";
        }
    });

    fiber.join();
    context.stop();
    io_thread.join();

    return result;
}
