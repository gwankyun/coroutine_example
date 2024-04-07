#include "data.hpp"

void on_read(error_code_t _error, std::size_t _bytes, std::shared_ptr<DataBase> _data);

void on_write(error_code_t _error, std::size_t _bytes, std::shared_ptr<DataBase> _data)
{
    auto &data = SocketData::from(_data);
    auto &socket = data.socket;
    auto &buffer = data.buffer;
    auto &offset = data.offset;
    auto remote_endpoint = socket.remote_endpoint();
    auto ip = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    if (_error)
    {
        SPDLOG_WARN("({} : {}) _error: {}", ip, port, _error.message());
        return;
    }

    SPDLOG_INFO("({} : {}) _bytes: {}", ip, port, _bytes);

    offset += _bytes;

    if (offset == buffer.size())
    {
        SPDLOG_WARN("({} : {}) write fininish!", ip, port);

        offset = 0;
        async_read(_data, 4, on_read);
    }
    else
    {
        async_write(_data, asio::buffer("", 0), on_write);
    }
}


void async_read(std::shared_ptr<DataBase> _data, std::size_t _size, on_callback_t _on_read)
{
    auto &data = SocketData::from(_data);
    auto &socket = data.socket;
    auto &buffer = data.buffer;
    auto &offset = data.offset;

    buffer.resize(offset + _size, '\0');
    socket.async_read_some(
        asio::buffer(buffer.data(), buffer.size()) += offset,
        [_data, _on_read](error_code_t _error, std::size_t _bytes)
        {
            _on_read(_error, _bytes, _data);
        });
}

void async_write(std::shared_ptr<DataBase> _data, asio::const_buffer _buffer, on_callback_t _on_write)
{
    auto &data = SocketData::from(_data);
    auto &socket = data.socket;
    auto &buffer = data.buffer;
    auto &offset = data.offset;

    if (_buffer.size() >= 0)
    {
        buffer.clear();
        offset = 0;

        std::copy_n((unsigned char *)(_buffer.data()), _buffer.size(), std::back_inserter(buffer));
    }

    socket.async_write_some(
        asio::buffer(buffer.data(), buffer.size()) += offset,
        [_data, _on_write](error_code_t _error, std::size_t _bytes)
        {
            _on_write(_error, _bytes, _data);
        });
}
