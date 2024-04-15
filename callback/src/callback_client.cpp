#include "callback.h"

void on_read(error_code_t _error, std::size_t _bytes,
    std::shared_ptr<TcpConnection> _connection)
{
    auto& conn = *_connection;
    auto& socket = conn.socket;
    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    auto& buffer = conn.buffer;
    auto& offset = conn.offset;

    SPDLOG_INFO("{} _bytes: {}", get_info(conn), _bytes);
    offset += _bytes;

    if (conn.read_done())
    {
        SPDLOG_INFO("{} : {}", get_info(conn),
            reinterpret_cast<const char*>(buffer.data()));
    }
    else
    {
        buffer.resize(offset + conn.per_read_size, 0xFF);
        socket.async_read_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            callback(_connection, on_read));
    }
}

void on_write(error_code_t _error, std::size_t _bytes,
    std::shared_ptr<TcpConnection> _connection)
{
    auto& conn = *_connection;
    auto& socket = conn.socket;
    auto& buffer = conn.buffer;
    auto& offset = conn.offset;
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
        buffer.resize(offset + conn.per_read_size, 0xFF);
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
    std::shared_ptr<TcpConnection> _connection)
{
    auto& conn = *_connection;
    auto& socket = conn.socket;
    auto& buffer = conn.buffer;
    auto& offset = conn.offset;

    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    offset = 0;
    buffer = to_buffer("client");
    socket.async_write_some(
        asio::buffer(buffer.data(), buffer.size()) += offset,
        callback(_connection, on_write));
}

int main(int _argc, char* _argv[])
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v");

    namespace ip = asio::ip;

    Argument argument;
    argument.parse_client(_argc, _argv);

    asio::io_context io_context;

    auto endpoint = ip::tcp::endpoint(argument.address, argument.port);

    std::vector<std::shared_ptr<TcpConnection>> conn_vec(argument.connect_number);

    int id = 0;
    for (auto& i : conn_vec)
    {
        i = std::make_shared<TcpConnection>(io_context);
        i->id = id;
        id++;
        i->socket.async_connect(
            endpoint,
            [=](error_code_t _error)
            {
                on_connect(_error, i);
            });
    }

    io_context.run();

    return 0;
}
