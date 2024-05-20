#include <chrono>
#include <queue>
#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>
#define BOOST_ALL_NO_LIB 1
#include <boost/asio.hpp>
#include <boost/context/fiber.hpp>

#include "on_exit.hpp"

namespace context = boost::context;

namespace type
{
    using context::fiber;
    using id = int;
}

namespace t = type;

namespace asio = boost::asio;

void resume(t::fiber& _fiber)
{
    _fiber = std::move(_fiber).resume();
}

void handle(asio::io_context& _io_context, bool _manage_on_sub, int _count)
{
    using fiber_ptr = std::unique_ptr<t::fiber>;

    fiber_ptr main;

    // 協程容器
    std::unordered_map<t::id, fiber_ptr> fiber_cont;

    // 待喚醒協程隊列。
    std::queue<t::id> awake_cont;

    auto create_fiber = [&](t::id id)
    {
        return [&, id](t::fiber&& _sink)
        {
            auto cb = [&, id] { awake_cont.push(id); };
            ON_EXIT(cb);
            std::vector<std::string> vec{
                "a",
                "b",
                "c",
            };
            for (auto& i : vec)
            {
                SPDLOG_INFO("id: {} value: {}", id, i);
                asio::post(_io_context, cb);
                resume(_sink);
            }
            return std::move(_sink);
        };
    };

    for (auto id = 0; id < _count; id++)
    {
        fiber_cont[id] = std::make_unique<t::fiber>(create_fiber(id));
        // 和continuation不一樣，創建後不會自動執行。
        auto& fiber = *fiber_cont[id];
        resume(fiber);
    }

    auto main_fiber = [&]
    {
        while (!fiber_cont.empty())
        {
            // 實際執行異步操作
            _io_context.run();

            if (!awake_cont.empty())
            {
                SPDLOG_DEBUG("awake_cont size: {}", awake_cont.size());
            }

            // 簡單的協程調度，按awake_cont中先進先出的順序喚醒協程。
            while (!awake_cont.empty())
            {
                auto i = awake_cont.front();
                awake_cont.pop();
                SPDLOG_DEBUG("awake id: {}", i);
                auto& cont = fiber_cont;
                auto iter = cont.find(i);
                if (iter != cont.end())
                {
                    auto& fiber = *(iter->second);
                    if (!fiber)
                    {
                        cont.erase(iter);
                        SPDLOG_DEBUG("child size: {}", cont.size());
                        continue;
                    }
                    resume(fiber);
                }
            }
        }
    };

    if (_manage_on_sub)
    {
        SPDLOG_INFO("create main");
        main = std::make_unique<t::fiber>(
            [&](t::fiber&& _sink)
            {
                SPDLOG_INFO("enter main");
                main_fiber();
                return std::move(_sink);
            });

        SPDLOG_INFO("resume main");
        resume(*main);
    }
    else
    {
        main_fiber();
    }
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;
    namespace chrono = std::chrono;

    auto start = chrono::steady_clock::now();
    handle(io_context, true, 3);
    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    SPDLOG_INFO("used times: {}", duration.count());

    return 0;
}
