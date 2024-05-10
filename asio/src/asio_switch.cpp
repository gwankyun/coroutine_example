#include <boost/asio.hpp>
#include <boost/system.hpp> // boost::system::error_code
#include <spdlog/spdlog.h>
#include <string>

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
using error_code_t = boost::system::error_code;

void handle(asio::io_context& _io_context, int _id, std::shared_ptr<std::vector<int>> _data, std::size_t _offset,
            std::shared_ptr<int> _state)
{
    CORO_BEGIN(*_state);
    while (_offset < _data->size())
    {
        SPDLOG_INFO("id: {} value: {}", _id, (*_data)[_offset]);
        asio::post(_io_context, [&_io_context, _id, _data, _offset, _state]
                   { handle(_io_context, _id, _data, _offset + 1, _state); });
        CORO_YIELD(*_state);
    }
    CORO_END();
}

void accept_handle(asio::io_context& _io_context, int& _id, int& _state)
{
    CORO_BEGIN(_state);
    while (_id < 3)
    {
        asio::post(_io_context, [&_io_context, &_id, &_state] { accept_handle(_io_context, _id, _state); });
        CORO_YIELD(_state);

        [&_io_context, _id]
        {
            auto _data = std::make_shared<std::vector<int>>();
            _data->push_back(1);
            _data->push_back(2);
            _data->push_back(3);

            auto state = std::make_shared<int>(0);

            asio::post(_io_context, [&_io_context, _id, _data, state] { handle(_io_context, _id, _data, 0, state); });
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
