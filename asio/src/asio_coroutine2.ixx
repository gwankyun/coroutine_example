module;
#define WIN32_LEAN_AND_MEAN
#include "use_module.h"

#if !USE_STD_MODULE
#  include <memory>
#  include <queue>
#  include <string>
#  include <unordered_map>
#  include <vector>
#endif

#include "catch2.h"

#include "spdlog.h"

//#define BOOST_LIB_DIAGNOSTIC
//#define BOOST_ALL_NO_LIB

#if !USE_BOOST_ASIO_MODULE
#  include <boost/asio.hpp>
#endif

#if !USE_BOOST_COROUTINE2_MODULE
#  include <boost/coroutine2/all.hpp>
#endif

#include <boost/system.hpp> // boost::system::error_code

//#include "time_count.h"

#if !USE_BOOST_SCOPE_MODULE
#  include <boost/scope/scope_exit.hpp>
#endif

export module asio_coroutines2;

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

#if USE_BOOST_COROUTINE2_MODULE
import boost.coroutine2;
#endif

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
    using output = std::unordered_map<id, std::string>;
} // namespace type

namespace t = type;

void handle(asio::io_context& _io_context, int _count, t::output& _output)
{
    using pull_ptr = std::unique_ptr<t::pull>;

    // 協程容器
    std::unordered_map<t::id, pull_ptr> coro_cont;

    // 待喚醒協程隊列。
    std::queue<t::id> awake_cont;

    auto create_coro = [&](t::id _id)
    {
        return [&, _id](t::push& _yield)
        {
            // 添加到喚醒隊列
            auto cb = [&, _id] { awake_cont.push(_id); };
            // 最後喚醒一次，讓管理協程得知已結束
            auto on_exit = boost::scope::make_scope_exit(cb);
            std::string buffer = "abc";
            for (auto& v : buffer)
            {
                _output[_id] += v;
                SPDLOG_INFO("id: {} value: {}", _id, v);

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
                if (!resume) // 是否執行結束
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

TEST_CASE("asio_corotinue2", "[corotinue2]")
{
    auto count = 3u;
    t::output output(count);

    asio::io_context io_context;

    handle(io_context, count, output);

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
