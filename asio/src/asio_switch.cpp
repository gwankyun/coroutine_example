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

void handle_old(error_code_t _error, int& _state, asio::io_context& _io_context, asio::steady_timer& _timer)
{
    auto cb = [_error, &_state, &_io_context, &_timer](error_code_t _e = error_code_t{})
    { handle_old(_e, _state, _io_context, _timer); };

    CORO_BEGIN(_state);
    asio::post(_io_context, cb);
    CORO_YIELD(_state);

    SPDLOG_INFO("read");
    asio::post(_io_context, cb);
    CORO_YIELD(_state);

    SPDLOG_INFO("write");
    _timer.async_wait([cb](error_code_t _e) { cb(_e); });
    CORO_YIELD(_state);

    if (_error)
    {
        SPDLOG_INFO("{}", _error.message());
        return;
    }
    SPDLOG_INFO("timeout");
    CORO_END();
}

void handle(asio::io_context& _io_context, int _id, std::shared_ptr<std::vector<int>> _data, std::size_t _offset,
            std::shared_ptr<int> _state)
{
    CORO_BEGIN(*_state);
    while (_offset < _data->size())
    {
        SPDLOG_INFO("id: {} value: {}", _id, (*_data)[_offset]);
        asio::post(_io_context,
                   [&_io_context, _id, _data, _offset, _state]
                   {
                       handle(_io_context, _id, _data, _offset + 1, _state);
                   });
        CORO_YIELD(*_state);
    }
    CORO_END();
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    asio::steady_timer timer(io_context, asio::chrono::seconds(1));

    // int state = 0;
    // handle_old(error_code_t{}, state, io_context, timer);

    for (auto i = 0; i < 3; i++)
    {
        auto vec = std::make_shared<std::vector<int>>();
        vec->push_back(1);
        vec->push_back(2);
        vec->push_back(3);

        auto state = std::make_shared<int>(0);

        handle(io_context, i, vec, 0, state);
    }

    io_context.run();

    return 0;
}
