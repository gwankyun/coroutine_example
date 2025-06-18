module;
#define WIN32_LEAN_AND_MEAN
#include "use_module.h"

#if !USE_STD_MODULE
#  include <memory>
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

#if !USE_BOOST_FIBER_MODULE
// https://github.com/boostorg/fiber/issues/314
#  define BOOST_FIBERS_STATIC_LINK
#  include <boost/fiber/all.hpp>
#endif

// #include "time_count.h"

#if !USE_BOOST_SCOPE_MODULE
#  include <boost/scope/defer.hpp>
#else
#  include <boost.scope/defer.hpp>
#endif

export module asio_fiber;

#if USE_STD_MODULE
import std;
#endif

#if USE_BOOST_ASIO_MODULE
import boost.asio;
namespace asio = boost_asio;
#endif

#if USE_CATCH2_MODULE
import catch2.compat;
#endif

#if USE_SPDLOG_MODULE
import spdlog;
#endif

#if USE_BOOST_SCOPE_MODULE
import boost.scope;
#endif

#if USE_BOOST_FIBER_MODULE
import boost.fiber;
#endif

namespace fibers = boost::fibers;
namespace this_fiber = boost::this_fiber;

namespace type
{
    using fibers::buffered_channel;
    using fibers::channel_op_status;
    using fibers::fiber;
    using id = int;
    using output = std::unordered_map<id, std::string>;
} // namespace type

namespace t = type;

namespace func
{
    void join(t::fiber& _fiber)
    {
        if (_fiber.joinable())
        {
            _fiber.join();
        }
    }
} // namespace func

namespace f = func;

void handle(asio::io_context& _io_context, int _count, t::output& _output)
{
    using fiber_ptr = std::unique_ptr<t::fiber>;

    fiber_ptr main;

    std::unordered_map<t::id, fiber_ptr> fiber_cont;

    t::buffered_channel<t::id> chan{2};

    BOOST_SCOPE_DEFER[&]
    {
        if (main)
        {
            f::join(*main);
        }

        for (auto& i : fiber_cont)
        {
            f::join(*(i.second));
        }
    };

    auto create_fiber = [&](t::id _id)
    {
        return [&, _id]
        {
            BOOST_SCOPE_DEFER[&, _id]
            {
                chan.push(_id);
            };
            std::string buffer = "abc";
            for (auto& v : buffer)
            {
                _output[_id] += v;
                SPDLOG_INFO("id: {} value: {}", _id, v);

                bool finished = false;
                asio::post(_io_context, [&] { finished = true; });
                while (!finished)
                {
                    SPDLOG_INFO("id: {} yield", _id);
                    this_fiber::yield();
                }
            }
        };
    };

    main = std::make_unique<t::fiber>(
        [&]
        {
            for (auto id = 0; id != _count; ++id)
            {
                fiber_cont[id] = std::make_unique<t::fiber>(create_fiber(id));
            }

            while (!fiber_cont.empty())
            {
                _io_context.run();
                t::id id = -1;
                if (chan.try_pop(id) == t::channel_op_status::success)
                {
                    auto& cont = fiber_cont;
                    auto iter = cont.find(id);
                    if (iter != cont.end())
                    {
                        f::join(*(iter->second));
                        cont.erase(iter);
                        SPDLOG_INFO("fiber_cont: {}", cont.size());
                    }
                }
                this_fiber::yield();
            }
        }
    );
}

TEST_CASE("asio_context_fiber", "[context_fiber]")
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
