#include <boost/asio.hpp>
#include <string>

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
}

namespace t = type;

struct Data
{
    std::vector<std::string> value;
    std::size_t offset = 0;
    t::id id;
    t::state state = 0;
};

void handle(asio::io_context& _io_context, std::shared_ptr<Data> _data)
{
    auto& data = *_data;
    auto& offset = data.offset;
    auto& value = data.value;
    auto& id = data.id;
    auto& state = data.state;

    CORO_BEGIN(state);
    while (offset < value.size())
    {
        SPDLOG_INFO("id: {} value: {}", id, value[offset]);
        offset += 1;
        asio::post(_io_context, [&, _data] { handle(_io_context, _data); });
        CORO_YIELD(state);
    }
    CORO_END();
}

void accept_handle(asio::io_context& _io_context, int _count, t::id& _id, t::state& _state)
{
    CORO_BEGIN(_state);
    while (_id < _count)
    {
        asio::post(_io_context, [&, _count] { accept_handle(_io_context, _count, _id, _state); });
        CORO_YIELD(_state);

        // switch內不能有局部變量。
        [&, _id]
        {
            auto data = std::make_shared<Data>();
            data->value = {"a", "b", "c"};
            data->id = _id;

            asio::post(_io_context, [&, data] { handle(_io_context, data); });
        }();

        _id++;
    }
    CORO_END();
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    auto count = common::time_count(
        [&]
        {
            t::id id = 0;
            t::state state = 0;
            accept_handle(io_context, 3, id, state);

            io_context.run();
        });

    SPDLOG_INFO("used times: {}", count);

    return 0;
}
