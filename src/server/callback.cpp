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
        std::string str{"server."};
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

    offset = 0;
    buffer.resize(1024, '\0');
    socket.async_read_some(
        asio::buffer(buffer.data(), buffer.size()) += offset,
        [&, _data](error_code_t _error, std::size_t _bytes)
        {
            on_read(_error, _bytes, _data);
        });
}

void on_accept(
    error_code_t _error,
    acceptor_t &_acceptor,
    std::shared_ptr<socket_t> _socket)
{
    if (_error)
    {
        SPDLOG_WARN(DBG(_error.message()));
        return;
    }

    auto newSocket = std::make_shared<socket_t>(_acceptor.get_executor());
    _acceptor.async_accept(
        *newSocket,
        [newSocket, &_acceptor](error_code_t error)
        {
            on_accept(error, _acceptor, newSocket);
        });

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
    using asio::ip::tcp;

    acceptor_t acceptor(
        io_context,
        tcp::endpoint(tcp::v4(), 8888));

    {
        auto socket = std::make_shared<socket_t>(io_context);
        acceptor.async_accept(
            *socket,
            [socket, &acceptor](error_code_t error)
            {
                on_accept(error, acceptor, socket);
            });
    }

    io_context.run();

    return 0;
}
