#include "data.hpp"

void on_write(error_code_t _error, std::size_t _bytes, std::shared_ptr<DataBase> _data)
{
    if (_error)
    {
        SPDLOG_WARN(DBG(_error.message()));
        return;
    }

    auto &data = SocketData::from(_data);
    auto &socket = data.socket;
    auto &buffer = data.buffer;
    auto &offset = data.offset;

    SPDLOG_INFO(DBG(_bytes));

    offset += _bytes;

    if (offset == buffer.size())
    {
        SPDLOG_INFO("write fininish!");

        offset = 0;
        buffer.resize(1024, '\0');
        socket.async_read_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            [&, _data](error_code_t _error, std::size_t _bytes)
            {
                on_read(_error, _bytes, _data);
            });
    }
    else
    {
        socket.async_write_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            [&, _data](error_code_t _error, std::size_t _bytes)
            {
                on_write(_error, _bytes, _data);
            });
    }
}
