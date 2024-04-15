#include "log.hpp"
#include "connection.h"

#define CORO_BEGIN(_state) \
    switch (_state)        \
    {                      \
    case 0:

#define CORO_YIELD(_state) \
    do                     \
    {                      \
        _state = __LINE__; \
        return;            \
    case __LINE__:;        \
    } while (false)

#define CORO_END() }

void handle_connection(
    error_code_t _error, std::size_t _bytes,
    acceptor_t &_acceptor,
    std::shared_ptr<Connection> _connection)
{
    auto &conn = *_connection;
    auto &socket = conn.socket;
    auto &buffer = conn.buffer;
    auto &offset = conn.offset;
    auto &state = conn.state;

    auto cb =
        [&, _connection](error_code_t _error, std::size_t _bytes = 0U)
    {
        handle_connection(_error, _bytes, _acceptor, _connection);
    };

    CORO_BEGIN(state);

    _acceptor.async_accept(socket, cb);
    CORO_YIELD(state);

    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    SPDLOG_INFO("{}", get_info(conn));

    // switch裡不能有局部變量，所以要作特殊處理。
    [&]
    {
        auto new_conn = std::make_shared<Connection>(_acceptor.get_executor());
        handle_connection(error_code_t{}, 0U, _acceptor, new_conn);
    }();

    offset = 0;
    while (offset == 0 || buffer[offset - 1] == '\0')
    {
        buffer.resize(offset + 4, '\0');
        socket.async_read_some(asio::buffer(buffer) += offset, cb);
        CORO_YIELD(state);

        if (_error)
        {
            SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
            return;
        }

        if (_bytes >= 0)
        {
            SPDLOG_INFO("{} _bytes: {}", get_info(conn), _bytes);
            offset += _bytes;
        }
    }

    SPDLOG_INFO("{} : {}",
                get_info(conn),
                reinterpret_cast<const char *>(buffer.data()));

    buffer.resize(offset);
    offset = 0;
    while (offset != buffer.size())
    {
        socket.async_write_some(asio::buffer(buffer) += offset, cb);
        CORO_YIELD(state);

        if (_error)
        {
            SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
            return;
        }

        if (_bytes >= 0)
        {
            offset += _bytes;
        }
    }

    SPDLOG_INFO("{} write fininish!", get_info(conn));

    CORO_END();
}

int main(int _argc, char *_argv[])
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v");

    namespace ip = asio::ip;
    using ip::tcp;

    Argument argument;
    argument.parse_server(_argc, _argv);

    SPDLOG_INFO("port: {}", argument.port);

    asio::io_context io_context;

    acceptor_t acceptor(
        io_context,
        tcp::endpoint(tcp::v4(), argument.port));

    {
        auto conn = std::make_shared<Connection>(io_context);
        handle_connection(error_code_t{}, 0U, acceptor, conn);
    }

    io_context.run();

    return 0;
}
