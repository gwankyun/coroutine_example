﻿#include <boost/asio.hpp>
#include <string>

#include <spdlog/spdlog.h>

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
}

namespace t = type;

struct Data
{
    std::vector<std::string> value;
    std::size_t offset = 0;
    t::id id;
    int state = 0;
};

void handle(asio::io_context& _io_context, std::shared_ptr<Data> _data)
{
    auto& offset = _data->offset;
    auto& value = _data->value;
    auto& id = _data->id;
    auto& state = _data->state;

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

void accept_handle(asio::io_context& _io_context, int& _id, int& _state)
{
    CORO_BEGIN(_state);
    while (_id < 3)
    {
        asio::post(_io_context, [&] { accept_handle(_io_context, _id, _state); });
        CORO_YIELD(_state);

        [&, _id]
        {
            auto data = std::make_shared<Data>();
            data->value.push_back("a");
            data->value.push_back("b");
            data->value.push_back("c");
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

    asio::steady_timer timer(io_context, asio::chrono::seconds(1));

    auto start = std::chrono::steady_clock::now();

    int id = 0;
    int state = 0;
    accept_handle(io_context, id, state);

    io_context.run();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    SPDLOG_INFO("used times: {}", duration.count());

    return 0;
}
