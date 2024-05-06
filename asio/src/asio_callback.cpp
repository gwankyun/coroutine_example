#include <boost/asio.hpp>
#include <boost/system.hpp> // boost::system::error_code
#include <spdlog/spdlog.h>
#include <string>
#include <memory>
#include <vector>

namespace asio = boost::asio;
using error_code_t = boost::system::error_code;

void on_wait(error_code_t _error)
{
    if (_error)
    {
        SPDLOG_INFO("{}", _error.message());
        return;
    }
    SPDLOG_INFO("timeout");
}

void on_write(asio::io_context& _io_context, asio::steady_timer& _timer)
{
    SPDLOG_INFO("write");
    _timer.async_wait([&_io_context, &_timer](error_code_t _error) { on_wait(_error); });
}

void on_read(asio::io_context& _io_context, asio::steady_timer& _timer)
{
    SPDLOG_INFO("read");
    asio::post(_io_context, [&_io_context, &_timer] { on_write(_io_context, _timer); });
}

void handle(asio::io_context& _io_context, int _id, std::shared_ptr<std::vector<int>> _data, std::size_t _offset)
{
    if (_offset < _data->size())
    {
        SPDLOG_INFO("id: {} value: {}", _id, (*_data)[_offset]);
        asio::post(_io_context,
                   [&_io_context, _id, _data, _offset]
                   {
                       handle(_io_context, _id, _data, _offset + 1);
                   });
    }
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    asio::steady_timer timer(io_context, asio::chrono::seconds(1));

    // asio::post(io_context, [&io_context, &timer] { on_read(io_context, timer); });

    for (auto i = 0; i < 3; i++)
    {
        auto vec = std::make_shared<std::vector<int>>();
        vec->push_back(1);
        vec->push_back(2);
        vec->push_back(3);

        handle(io_context, i, vec, 0);
    }

    io_context.run();

    return 0;
}
