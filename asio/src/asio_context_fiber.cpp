#include <chrono>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#define BOOST_LIB_DIAGNOSTIC
#define BOOST_ALL_NO_LIB
#include <boost/asio.hpp>
#include <boost/context/fiber.hpp>

#include "on_exit.h"
#include "time_count.h"

namespace context = boost::context;

namespace type
{
    using context::fiber;
    using id = int;
} // namespace type

namespace t = type;

namespace asio = boost::asio;

namespace func
{
    void resume(t::fiber& _fiber)
    {
        _fiber = std::move(_fiber).resume();
    }
} // namespace func

namespace f = func;

void handle(asio::io_context& _io_context, int _count, bool _manage_on_sub, std::vector<std::string>& _output)
{
    using fiber_ptr = std::unique_ptr<t::fiber>;

    fiber_ptr main;

    // 協程容器
    std::unordered_map<t::id, fiber_ptr> fiber_cont;

    // 待喚醒協程隊列。
    std::queue<t::id> awake_cont;

    auto create_fiber = [&](t::id _id)
    {
        return [&, _id](t::fiber&& _sink)
        {
            auto cb = [&, _id] { awake_cont.push(_id); };
            ON_EXIT(cb);
            std::vector<std::string> vec{
                "a",
                "b",
                "c",
            };
            for (auto& i : vec)
            {
                asio::post(_io_context, cb);
                f::resume(_sink);

                SPDLOG_INFO("id: {} value: {}", _id, i);
                _output[_id] += i;
            }
            return std::move(_sink);
        };
    };

    auto main_fiber = [&]
    {
        for (auto id = 0; id < _count; id++)
        {
            fiber_cont[id] = std::make_unique<t::fiber>(create_fiber(id));
            // 和continuation不一樣，創建後不會自動執行。
            auto& fiber = *fiber_cont[id];
            f::resume(fiber);
        }

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
                    f::resume(fiber);
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
        f::resume(*main);
    }
    else
    {
        main_fiber();
    }
}

TEST_CASE("asio_context_fiber", "[context_fiber]")
{
    auto count = 3u;
    std::vector<std::string> output(count);

    asio::io_context io_context;

    handle(io_context, count, true, output);

    for (auto& i : output)
    {
        REQUIRE(i == "abc");
    }
}

int main(int argc, char* argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(argc, argv);
    return result;
}
