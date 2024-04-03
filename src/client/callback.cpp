#include "data.hpp"

void on_read(error_code_t _error, std::size_t _bytes, std::shared_ptr<DataBase> _data)
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

    if (buffer[offset - 1] == '.')
    {
        SPDLOG_INFO(DBG(buffer.data()));
        socket.close();
    }
    else
    {
        socket.async_read_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            [&, _data](error_code_t _error, std::size_t _bytes)
            {
                on_read(_error, _bytes, _data);
            });
    }
}

void callback(std::shared_ptr<DataBase> _data)
{
    auto &data = SocketData::from(_data);
    auto &socket = data.socket;
    auto &buffer = data.buffer;
    auto &offset = data.offset;

    std::string str{"client."};
    buffer.clear();
    std::copy_n(str.begin(), str.size(), std::back_inserter(buffer));
    offset = 0;

    socket.async_write_some(
        asio::buffer(buffer.data(), buffer.size()) += offset,
        [&, _data](error_code_t _error, std::size_t _bytes)
        {
            on_write(_error, _bytes, _data);
        });
}

void on_connect(
    error_code_t _error,
    std::shared_ptr<socket_t> _socket)
{
    if (_error)
    {
        SPDLOG_WARN(DBG(_error.message()));
        return;
    }

    auto remote_endpoint = _socket->remote_endpoint();
    auto ip = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    SPDLOG_INFO(DBG(ip));
    SPDLOG_INFO(DBG(port));

    callback(std::make_shared<SocketData>(std::move(*_socket))); // 回調版本
}

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v");

    asio::io_context io_context;

    namespace ip = asio::ip;

    {
        auto socket = std::make_shared<socket_t>(io_context);
        auto address = ip::make_address("127.0.0.1");
        socket->async_connect(
            ip::tcp::endpoint(address, 8888),
            [socket](error_code_t _error)
            {
                on_connect(_error, socket);
            });
    }

    io_context.run();

    return 0;
}
