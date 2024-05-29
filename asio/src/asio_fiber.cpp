#include <memory>
#include <string>
#include <unordered_map>

#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#define BOOST_LIB_DIAGNOSTIC
#define BOOST_ALL_NO_LIB
#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>

#include "on_exit.h"
#include "time_count.h"

namespace asio = boost::asio;

namespace fibers = boost::fibers;
namespace this_fiber = boost::this_fiber;

namespace type
{
    using fibers::buffered_channel;
    using fibers::channel_op_status;
    using fibers::fiber;
    using id = int;
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

void handle(asio::io_context& _io_context, int _count, std::vector<std::string>& _output)
{
    using fiber_ptr = std::unique_ptr<t::fiber>;

    fiber_ptr main;

    std::unordered_map<t::id, fiber_ptr> fiber_cont;

    t::buffered_channel<t::id> chan{2};

    ON_EXIT(
        [&]
        {
            if (main)
            {
                f::join(*main);
            }

            for (auto& i : fiber_cont)
            {
                f::join(*(i.second));
            }
        });

    auto create_fiber = [&](t::id _id)
    {
        return [&, _id]
        {
            ON_EXIT([&, _id] { chan.push(_id); });
            std::vector<std::string> vec{
                "a",
                "b",
                "c",
            };
            for (auto& i : vec)
            {
                bool finished = false;
                asio::post(_io_context, [&] { finished = true; });
                while (!finished)
                {
                    SPDLOG_INFO("id: {} yield", _id);
                    this_fiber::yield();
                }

                SPDLOG_INFO("id: {} value: {}", _id, i);
                _output[_id] += i;
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
        });
}

TEST_CASE("asio_context_fiber", "[context_fiber]")
{
    auto count = 3u;
    std::vector<std::string> output(count);

    asio::io_context io_context;

    handle(io_context, count, output);

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
