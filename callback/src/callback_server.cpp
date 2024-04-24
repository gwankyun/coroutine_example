﻿#include "callback.h"

void on_write(error_code_t _error, std::size_t _bytes, std::shared_ptr<Connection> _connection);

void on_read(error_code_t _error, std::size_t _bytes, std::shared_ptr<Connection> _connection)
{
    auto& conn = *_connection;
    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    auto& socket = conn.socket;
    auto& buffer = conn.buffer;
    auto& offset = conn.offset;

    SPDLOG_INFO("{} _bytes: {}", get_info(conn), _bytes);
    if (_bytes > 0)
    {
        offset += _bytes;
    }

    if (offset == 0 || buffer[offset - 1] == '\0')
    {
        SPDLOG_INFO("{} : {}", get_info(conn), reinterpret_cast<const char*>(buffer.data()));

        buffer.resize(offset);
        offset = 0;
        socket.async_write_some(asio::buffer(buffer.data(), buffer.size()) += offset, callback(_connection, on_write));
    }
    else
    {
        buffer.resize(offset + 4, '\0');
        socket.async_read_some(asio::buffer(buffer.data(), buffer.size()) += offset, callback(_connection, on_read));
    }
}

void on_write(error_code_t _error, std::size_t _bytes, std::shared_ptr<Connection> _connection)
{
    auto& conn = *_connection;
    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    auto& socket = conn.socket;
    auto& buffer = conn.buffer;
    auto& offset = conn.offset;

    SPDLOG_INFO("{} _bytes: {}", get_info(conn), _bytes);
    if (_bytes > 0)
    {
        offset += _bytes;
    }

    if (offset == buffer.size())
    {
        SPDLOG_INFO("{} write fininish!", get_info(conn));

        offset = 0;
        buffer.resize(offset + 4, 0xFF);
        socket.async_read_some(asio::buffer(buffer.data(), buffer.size()) += offset, callback(_connection, on_read));
    }
    else
    {
        socket.async_write_some(asio::buffer(buffer.data(), buffer.size()) += offset, callback(_connection, on_write));
    }
}

void on_accept(error_code_t _error, acceptor_t& _acceptor, std::shared_ptr<Connection> _connection)
{
    auto& conn = *_connection;
    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    auto& socket = conn.socket;
    auto& buffer = conn.buffer;
    auto& offset = conn.offset;

    auto new_conn = std::make_shared<Connection>(_acceptor.get_executor());
    _acceptor.async_accept(new_conn->socket,
                           [new_conn, &_acceptor](error_code_t error) { on_accept(error, _acceptor, new_conn); });

    SPDLOG_INFO("{}", get_info(conn));

    offset = 0;
    buffer.resize(offset + 4, 0xFF);
    socket.async_read_some(asio::buffer(buffer.data(), buffer.size()) += offset, callback(_connection, on_read));
}

int main(int _argc, char* _argv[])
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v");

    asio::io_context io_context;
    using asio::ip::tcp;

    Argument argument;
    argument.parse_server(_argc, _argv);

    acceptor_t acceptor(io_context, tcp::endpoint(tcp::v4(), argument.port));

    {
        auto conn = std::make_shared<Connection>(io_context);
        acceptor.async_accept(conn->socket,
                              [conn, &acceptor](error_code_t error) { on_accept(error, acceptor, conn); });
    }

    io_context.run();

    return 0;
}
