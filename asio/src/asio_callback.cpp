#include <boost/asio.hpp>
#include <boost/system.hpp> // boost::system::error_code
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace asio = boost::asio;
using error_code_t = boost::system::error_code;

void handle(asio::io_context& _io_context, int _id, std::shared_ptr<std::vector<int>> _data, std::size_t _offset)
{
    if (_offset < _data->size())
    {
        SPDLOG_INFO("id: {} value: {}", _id, (*_data)[_offset]);
        asio::post(_io_context, [&_io_context, _id, _data, _offset] { handle(_io_context, _id, _data, _offset + 1); });
    }
}

void accept_handle(asio::io_context& _io_context, int _id)
{
    if (_id < 3)
    {
        auto vec = std::make_shared<std::vector<int>>();
        vec->push_back(1);
        vec->push_back(2);
        vec->push_back(3);

        asio::post(_io_context, [&_io_context, _id, vec] { handle(_io_context, _id, vec, 0); });

        asio::post(_io_context, [&_io_context, _id] { accept_handle(_io_context, _id + 1); });
    }
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    asio::steady_timer timer(io_context, asio::chrono::seconds(1));

    asio::post(io_context, [&io_context] { accept_handle(io_context, 0); });

    io_context.run();

    return 0;
}
