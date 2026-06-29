module;

#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>
#include <spdlog/spdlog.h>

// 引入適配asio的調度器
#include "fiber/asio/round_robin.hpp"

module test.fiber;

import std;

using namespace std::chrono_literals;
namespace asio = boost::asio;
using asio::steady_timer;
using boost::system::error_code;
namespace fibers = boost::fibers;

std::unordered_map<int, std::string> test_fiber()
{
    auto context = std::make_shared<asio::io_context>();
    // 啟用asio調度器
    fibers::use_scheduling_algorithm<fibers::asio::round_robin>(context);

    std::unordered_map<int, std::string> result;
    auto time = 100ms;

    auto make_fiber = [&](int id) {
        fibers::fiber fiber([&, id] {
            error_code ec;

            steady_timer connection(*context);
            connection.expires_after(time);
            connection.async_wait(fibers::asio::yield[ec]);
            if (ec)
            {
                SPDLOG_ERROR("{}", ec.message());
                return;
            }

            result[id] = std::to_string(id);

            for (int i = 0; i < 3; ++i)
            {
                // steady_timer read(*context, time);
                connection.expires_after(time);
                // 可以自動切走切回
                connection.async_wait(fibers::asio::yield[ec]);
                if (ec)
                {
                    SPDLOG_ERROR("{}", ec.message());
                    return;
                }
                result[id] += "r";
            }

            // steady_timer write(*context, time);
            connection.expires_after(time);
            connection.async_wait(fibers::asio::yield[ec]);
            if (ec)
            {
                SPDLOG_ERROR("{}", ec.message());
                return;
            }
            result[id] += "w";
        });

        return std::move(fiber);
    };

    // 主fiber，用來管理別的fiber
    fibers::fiber main_fiber([&] {
        std::vector<fibers::fiber> fiber_cont;

        // steady_timer accept(*context, time);
        steady_timer acceptor(*context);

        for (auto i = 0; i != 3; ++i)
        {
            error_code ec;

            acceptor.expires_after(time);
            acceptor.async_wait(fibers::asio::yield[ec]);
            if (ec)
            {
                SPDLOG_ERROR("{}", ec.message());
                return;
            }

            SPDLOG_INFO("new fiber: {}", i);

            // 動態創建子fiber
            auto fiber = make_fiber(i);
            fiber_cont.push_back(std::move(fiber));
        }

        // 等待所有子fiber結束
        for (auto& f : fiber_cont)
        {
            f.join();
        }

        // 最後一個fiber需要先停止io_context
        context->stop();
    });

    main_fiber.detach();

    context->run();
    return result;
}
