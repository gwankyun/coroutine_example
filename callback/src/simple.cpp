﻿#include "log.hpp"
#include "connection.h"
#include <functional>

using callback_t = void (*)(error_code_t _error, std::size_t _bytes, std::shared_ptr<ConnectionBase> _connection);

auto callback =
    [](std::shared_ptr<ConnectionBase> _connection, callback_t _cb)
{
    return [_connection, _cb](error_code_t _error, std::size_t _bytes)
    {
        _cb(_error, _bytes, _connection);
    };
};

void on_write(error_code_t _error, std::size_t _bytes, std::shared_ptr<ConnectionBase> _connection);

void on_read(error_code_t _error, std::size_t _bytes, std::shared_ptr<ConnectionBase> _connection)
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

        offset = 0;
        buffer = conn.get_write_buffer();
        socket.async_write_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            callback(_connection, on_write));
    }
    else
    {
        buffer.resize(offset + conn.per_read_size, '\0');
        socket.async_read_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            callback(_connection, on_read));
    }
}

void on_write(error_code_t _error, std::size_t _bytes, std::shared_ptr<ConnectionBase> _connection)
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

void on_accept(
    error_code_t _error,
    acceptor_t &_acceptor,
    std::shared_ptr<ConnectionBase> _connection)
{
    if (_error)
    {
        SPDLOG_WARN(DBG(_error.message()));
        return;
    }
    auto &conn = TcpConnection::from(_connection);
    auto &socket = conn.socket;
    auto &buffer = conn.buffer;
    auto &offset = conn.offset;

    auto newData = std::make_shared<TcpConnection>(_acceptor.get_executor());
    _acceptor.async_accept(
        newData->socket,
        [newData, &_acceptor](error_code_t error)
        {
            on_accept(error, _acceptor, newData);
        });

    SPDLOG_INFO("{}", get_info(conn));

    offset = 0;
    buffer.resize(offset + conn.per_read_size, '\0');
    socket.async_read_some(
        asio::buffer(buffer.data(), buffer.size()) += offset,
        callback(_connection, on_read));
}

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v");

    asio::io_context io_context;
    using asio::ip::tcp;

    acceptor_t acceptor(
        io_context,
        tcp::endpoint(tcp::v4(), 8888));

    {
        auto conn = std::make_shared<TcpConnection>(io_context);
        acceptor.async_accept(
            conn->socket,
            [conn, &acceptor](error_code_t error)
            {
                on_accept(error, acceptor, conn);
            });
    }

    io_context.run();

    return 0;
}