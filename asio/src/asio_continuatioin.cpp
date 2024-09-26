﻿#include <queue>
#include <unordered_map>
#include <string>

#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#define BOOST_LIB_DIAGNOSTIC
#define BOOST_ALL_NO_LIB
#include <boost/asio.hpp>
#include <boost/context/continuation.hpp>

#include "on_exit.h"
#include "time_count.h"

namespace context = boost::context;

namespace type
{
    using continuation = context::continuation;
    using id = int;
    using output = std::unordered_map<id, std::string>;
} // namespace type

namespace t = type;

namespace asio = boost::asio;

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
            }));
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

int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
