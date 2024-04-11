#pragma once
#include <vector>
#include <boost/system.hpp> // boost::system::error_code
#include <boost/asio/buffer.hpp>

using error_code_t = boost::system::error_code;
namespace asio = boost::asio;

struct ConnectionBase
{
    ConnectionBase() = default;
    virtual ~ConnectionBase() = default;
    using buffer_t = std::vector<char>;
    buffer_t buffer;
    std::size_t offset = 0;
};

using on_callback_t = void (*)(error_code_t _error, std::size_t _bytes, std::shared_ptr<ConnectionBase> _data);

void on_write(error_code_t _error, std::size_t _bytes, std::shared_ptr<ConnectionBase> _data);
void async_read(std::shared_ptr<ConnectionBase> _data, std::size_t _size, on_callback_t _on_read);
void async_write(std::shared_ptr<ConnectionBase> _data, boost::asio::const_buffer _buffer, on_callback_t _on_write);
