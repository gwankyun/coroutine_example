#include <string>
#include <vector>
#include <unordered_map>

#include <boost/asio.hpp>
#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>

#include "time_count.h"

// clang-format off
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
// clang-format on

namespace asio = boost::asio;

namespace type
{
    using id = int;
    using state = int;
    using output = std::unordered_map<id, std::string>;
} // namespace type

namespace t = type;

struct Data
{
    std::vector<std::string> value;
    std::size_t offset = 0;
    t::id id;
    t::state state = 0;
};

void handle(asio::io_context& _io_context, std::shared_ptr<Data> _data, t::output& _output)
{
    auto& data = *_data;
    auto& offset = data.offset;
    auto& value = data.value;
    auto& id = data.id;
    auto& state = data.state;

    CORO_BEGIN(state);
    for (; offset < value.size(); offset++)
    {
        asio::post(_io_context, [&, _data] { handle(_io_context, _data, _output); });
        CORO_YIELD(state);

        SPDLOG_INFO("id: {} value: {}", id, value[offset]);
        _output[id] += value[offset];
    }
    CORO_END();
}

void accept_handle(asio::io_context& _io_context, int _count, t::id& _id, t::state& _state,
                   t::output& _output)
{
    CORO_BEGIN(_state);
    for (; _id < _count; _id++)
    {
        asio::post(_io_context, [&, _count] { accept_handle(_io_context, _count, _id, _state, _output); });
        CORO_YIELD(_state);

        // switch內不能有局部變量。
        [&, _id]
        {
            auto data = std::make_shared<Data>();
            data->value = {"a", "b", "c"};
            data->id = _id;

            asio::post(_io_context, [&, data] { handle(_io_context, data, _output); });
        }();
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

int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
