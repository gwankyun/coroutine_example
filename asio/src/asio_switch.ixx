module;
#include "use_module.h"

#if !USE_STD_MODULE
#  include <string>
#  include <unordered_map>
#  include <vector>
#endif

#include "catch2.h"

#include <spdlog/spdlog.h>

#if !USE_BOOST_ASIO_MODULE
#  include <boost/asio.hpp>
#endif

// #include "time_count.h"

export module asio_switch;

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

#define CORO_BEGIN(_state) \
    switch (_state) \
    { \
    case 0:

#define CORO_YIELD(_state) \
    do \
    { \
        _state = __LINE__; \
        return; \
    case __LINE__:; \
    } while (false)

#define CORO_END() }

namespace type
{
    using id = int;
    using state = int;
    using output = std::unordered_map<id, std::string>;
} // namespace type

namespace t = type;

struct Data
{
    t::id id;
    std::string value;
    std::size_t offset = 0;
    t::state state = 0;
};

void handle(asio::io_context& _io_context, std::shared_ptr<Data> _data, t::output& _output)
{
    auto& data = *_data;
    auto& value = data.value;
    auto& id = data.id;
    auto& state = data.state;
    auto& offset = data.offset;

    CORO_BEGIN(state);
    for (offset = 0; offset < value.size(); offset++)
    {
        [&]
        {
            auto v = value[offset];
            _output[id] += v;
            SPDLOG_INFO("id: {} value: {}", id, v);
        }();

        asio::post(_io_context, [&, _data] { handle(_io_context, _data, _output); });
        CORO_YIELD(state);
    }
    CORO_END();
}

void accept_handle(asio::io_context& _io_context, int _count, t::id& _id, t::state& _state, t::output& _output)
{
    CORO_BEGIN(_state);
    for (_id = 0; _id < _count; _id++)
    {
        // switch內不能有局部變量。
        [&, _id]
        {
            auto data = std::make_shared<Data>();
            data->value = "abc";
            data->id = _id;

            handle(_io_context, data, _output);
        }();

        asio::post(_io_context, [&, _count] { accept_handle(_io_context, _count, _id, _state, _output); });
        CORO_YIELD(_state);
    }
    CORO_END();
}

TEST_CASE("asio_switch", "[switch]")
{
    auto count = 3u;
    t::output output(count);

    asio::io_context io_context;

    t::id id = 0;
    t::state state = 0;
    accept_handle(io_context, count, id, state, output);

    io_context.run();

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
