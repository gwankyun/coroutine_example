﻿#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>
#define BOOST_ALL_NO_LIB 1
#include <boost/asio.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/system.hpp> // boost::system::error_code

#include "on_exit.hpp"
#include "time.hpp"

namespace asio = boost::asio;

namespace coro2 = boost::coroutines2;

namespace type
{
    template <typename T = void>
    using pull_type = typename coro2::coroutine<T>::pull_type;

    template <typename T = void>
    using push_type = typename coro2::coroutine<T>::push_type;

    using pull = pull_type<>;
    using push = push_type<>;
    using id = int;
} // namespace type

namespace t = type;

void handle(asio::io_context& _io_context, int _count)
{
    // 協程容器
    std::unordered_map<t::id, std::unique_ptr<t::pull>> coro_cont;

    // 待喚醒協程隊列。
    std::queue<t::id> awake_cont;

    auto create_coro = [&](t::id id)
    {
        return [&, id](t::push& _yield)
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
                _yield();
            }
        };
    };

    for (auto id = 0; id < _count; id++)
    {
        coro_cont[id] = std::make_unique<t::pull>(create_coro(id));
    }

    while (!coro_cont.empty())
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
            auto& cont = coro_cont;
            auto iter = cont.find(i);
            if (iter != cont.end())
            {
                auto& resume = *(iter->second);
                if (!resume)
                {
                    cont.erase(iter);
                    SPDLOG_DEBUG("child size: {}", cont.size());
                    continue;
                }
                resume();
            }
        }
    }
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    auto count = common::time_count([&] { handle(io_context, 3); });

    SPDLOG_INFO("used times: {}", count);

    return 0;
}
