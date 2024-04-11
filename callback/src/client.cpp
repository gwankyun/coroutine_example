#include "callback.h"

void on_read(error_code_t _error, std::size_t _bytes,
             std::shared_ptr<ConnectionBase> _connection)
{
    auto &conn = TcpConnection::from(_connection);
    auto &socket = conn.socket;
    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    auto &buffer = conn.buffer;
    auto &offset = conn.offset;

    SPDLOG_INFO("{} _bytes: {}", get_info(conn), _bytes);
    offset += _bytes;

    if (conn.read_done())
    {
        SPDLOG_INFO("{} : {}", get_info(conn), buffer.data());
    }
    else
    {
        buffer.resize(offset + conn.per_read_size, '\0');
        socket.async_read_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            callback(_connection, on_read));
    }
}

void on_write(error_code_t _error, std::size_t _bytes,
              std::shared_ptr<ConnectionBase> _connection)
{
    auto &conn = TcpConnection::from(_connection);
    auto &socket = conn.socket;
    auto &buffer = conn.buffer;
    auto &offset = conn.offset;
    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    SPDLOG_INFO("{} _bytes: {}", get_info(conn), _bytes);

    offset += _bytes;

    if (offset == buffer.size())
    {
        SPDLOG_INFO("{} write fininish!", get_info(conn));

        offset = 0;
        buffer.resize(offset + conn.per_read_size, '\0');
        socket.async_read_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            callback(_connection, on_read));
    }
    else
    {
        socket.async_write_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            callback(_connection, on_write));
    }
}

void on_connect(
    error_code_t _error,
    std::shared_ptr<ConnectionBase> _connection)
{
    auto &conn = TcpConnection::from(_connection);
    auto &socket = conn.socket;
    auto &buffer = conn.buffer;
    auto &offset = conn.offset;

    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    offset = 0;
    buffer = to_buffer("client.");
    socket.async_write_some(
        asio::buffer(buffer.data(), buffer.size()) += offset,
        callback(_connection, on_write));
}

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v");

    asio::io_context io_context;

    namespace ip = asio::ip;
    auto address = ip::make_address("127.0.0.1");
    auto endpoint = ip::tcp::endpoint(address, 8888);

    std::vector<std::shared_ptr<TcpConnection>> conn_vec(2);

    for (auto &i : conn_vec)
    {
        i = std::make_shared<TcpConnection>(io_context);
        i->socket.async_connect(
            endpoint,
            [i](error_code_t _error)
            {
                on_connect(_error, i);
            });
    }

    io_context.run();

    return 0;
}
