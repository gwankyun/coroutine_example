module;
#include "use_module.h"

#if !USE_STD_MODULE
#  include <chrono>
#  include <queue>
#  include <string>
#  include <unordered_map>
#  include <vector>
#endif

#include "catch2.h"

#include <spdlog/spdlog.h>

//#define BOOST_LIB_DIAGNOSTIC
//#define BOOST_ALL_NO_LIB

#if !USE_BOOST_ASIO_MODULE
#  include <boost/asio.hpp>
#endif

#if !USE_BOOST_CONTEXT_MODULE
#  include <boost/context/fiber.hpp>
#endif

//#include "time_count.h"

#if !USE_BOOST_SCOPE_MODULE
#  include <boost/scope/scope_exit.hpp>
#endif

export module asio_context_fiber;

#if USE_STD_MODULE
import std;
#endif

#if USE_CATCH2_MODULE
import catch2.compat;
#endif

#if USE_SPDLOG_MODULE
import spdlog;
#endif

#if USE_BOOST_CONTEXT_MODULE
import boost.context;
#endif

#if USE_BOOST_ASIO_MODULE
import boost.asio;
namespace asio = boost_asio;
#endif

#if USE_BOOST_SCOPE_MODULE
import boost.scope;
#endif

namespace context = boost::context;

namespace type
{
    using context::fiber;
    using id = int;
    using output = std::unordered_map<id, std::string>;
} // namespace type

namespace t = type;

namespace func
{
    void resume(t::fiber& _fiber)
    {
        _fiber = std::move(_fiber).resume();
    }
} // namespace func

namespace f = func;

void handle(asio::io_context& _io_context, int _count, bool _manage_on_sub, t::output& _output)
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
            auto on_exit = boost::scope::make_scope_exit(cb);
            std::string buffer = "abc";
            for (auto& v : buffer)
            {
                _output[_id] += v;
                SPDLOG_INFO("id: {} value: {}", _id, v);

                asio::post(_io_context, cb);
                f::resume(_sink);
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
            }
        );

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
    t::output output(count);

    asio::io_context io_context;

    handle(io_context, count, true, output);

    for (auto& i : output)
    {
        REQUIRE(i.second == "abc");
    }
}

export int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
