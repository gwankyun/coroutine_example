module;
#include "use_module.h"

#if !USE_STD_MODULE
#  include <queue>
#  include <string>
#  include <unordered_map>
#endif

#include "catch2.h"

#include <spdlog/spdlog.h>

// #define BOOST_LIB_DIAGNOSTIC
// #define BOOST_ALL_NO_LIB

#if !USE_BOOST_ASIO_MODULE
#  include <boost/asio.hpp>
#endif

#if !USE_BOOST_CONTEXT_MODULE
#  include <boost/context/continuation.hpp>
#endif

// #include "time_count.h"

#if !USE_BOOST_SCOPE_MODULE
#  include <boost/scope/scope_exit.hpp>
#endif

export module asio_continuatioin;

#if USE_STD_MODULE
import std;
#endif

#if USE_CATCH2_MODULE
import catch2.compat;
#endif

#if USE_SPDLOG_MODULE
import spdlog;
#endif

#if USE_BOOST_ASIO_MODULE
import boost.asio;
namespace asio = boost_asio;
#endif

#if USE_BOOST_SCOPE_MODULE
import boost.scope;
#endif

#if USE_BOOST_CONTEXT_MODULE
import boost.context;
#endif

namespace context = boost::context;

namespace type
{
    using continuation = context::continuation;
    using id = int;
    using output = std::unordered_map<id, std::string>;
} // namespace type

namespace t = type;
namespace func
{
    void resume(t::continuation& _continuation)
    {
        _continuation = _continuation.resume();
    }
} // namespace func

namespace f = func;

void handle(asio::io_context& _io_context, int _count, bool _manage_on_sub, t::output& _output)
{
    using continuation_ptr = std::unique_ptr<t::continuation>;

    continuation_ptr main;

    // 協程容器
    std::unordered_map<t::id, continuation_ptr> continuation_cont;

    // 待喚醒協程隊列。
    std::queue<t::id> awake_cont;

    auto create_continuation = [&](t::id _id)
    {
        auto continuation = [&, _id](t::continuation&& _sink)
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
        return context::callcc(continuation);
    };

    auto main_continuation = [&]
    {
        for (auto id = 0; id < _count; id++)
        {
            continuation_cont[id] = std::make_unique<t::continuation>(create_continuation(id));
        }

        while (!continuation_cont.empty())
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
                auto& cont = continuation_cont;
                auto iter = cont.find(i);
                if (iter != cont.end())
                {
                    auto& continuation = *(iter->second);
                    if (!continuation)
                    {
                        cont.erase(iter);
                        SPDLOG_DEBUG("child size: {}", cont.size());
                        continue;
                    }
                    f::resume(continuation);
                }
            }
        }
    };

    if (_manage_on_sub)
    {
        SPDLOG_INFO("create main");
        SPDLOG_INFO("resume main");
        main = std::make_unique<t::continuation>(context::callcc(
            [&](t::continuation&& _sink)
            {
                SPDLOG_INFO("enter main");
                main_continuation();
                return std::move(_sink);
            }
        ));
    }
    else
    {
        main_continuation();
    }
}

TEST_CASE("asio_continuation", "[continuation]")
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
