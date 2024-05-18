#include <memory>
#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>
#define BOOST_ALL_NO_LIB 1
#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>

#include "on_exit.hpp"

namespace asio = boost::asio;

namespace fibers = boost::fibers;

namespace type
{
    using fibers::buffered_channel;
    using fibers::channel_op_status;
    using fibers::fiber;
    using id = int;
}

namespace t = type;

void join(t::fiber& _fiber)
{
    if (_fiber.joinable())
    {
        _fiber.join();
    }
}

void handle(asio::io_context& _io_context)
{
    std::unique_ptr<t::fiber> main;

    std::unordered_map<t::id, std::unique_ptr<t::fiber>> fiber_cont;

    t::buffered_channel<t::id> chan{2};

    ON_EXIT(
        [&]
        {
            if (main)
            {
                join(*main);
            }

            for (auto& i : fiber_cont)
            {
                join(*(i.second));
            }
        });

    auto create_fiber = [&](t::id id)
    {
        return [&, id]
        {
            ON_EXIT([&, id] { chan.push(id); });
            std::vector<std::string> vec{
                "a",
                "b",
                "c",
            };
            for (auto& i : vec)
            {
                SPDLOG_INFO("id: {} value: {}", id, i);
                bool finished = false;
                asio::post(_io_context, [&] { finished = true; });
                while (!finished)
                {
                    boost::this_fiber::yield();
                }
            }
        };
    };

    main = std::make_unique<t::fiber>(
        [&]
        {
            for (auto id = 0; id != 3; ++id)
            {
                fiber_cont[id] = std::make_unique<t::fiber>(create_fiber(id));
            }

            while (!fiber_cont.empty())
            {
                _io_context.run();
                int id = -1;
                if (chan.try_pop(id) == t::channel_op_status::success)
                {
                    auto iter = fiber_cont.find(id);
                    if (iter != fiber_cont.end())
                    {
                        join(*(iter->second));
                        fiber_cont.erase(iter);
                        SPDLOG_INFO("fiber_cont: {}", fiber_cont.size());
                    }
                }
                boost::this_fiber::yield();
            }
        });
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    handle(io_context);

    return 0;
}
