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

void accept_socket(
    error_code_t _error,
    std::size_t _bytes,
    acceptor_t &_acceptor,
    std::shared_ptr<ConnectionBase> _connection)
{
    auto &data = TcpConnection::from(_connection);
    auto &socket = data.socket;
    auto &buffer = data.buffer;
    auto &offset = data.offset;
    auto &state = data.state;

    auto cb =
        [&, _connection](error_code_t _error, std::size_t _bytes = 0U)
    {
        accept_socket(_error, _bytes, _acceptor, _connection);
    };

    CORO_BEGIN(state);

    _acceptor.async_accept(socket, cb);
    CORO_YIELD(state);

    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(data), _error.message());
        return;
    }

    SPDLOG_INFO("{}", get_info(data));

    // switch裡不能有局部變量，所以要作特殊處理。
    [&]
    {
        auto newData = std::make_shared<TcpConnection>(_acceptor.get_executor());
        accept_socket(error_code_t{}, 0U, _acceptor, newData);
    }();

    offset = 0;
    while (!data.read_done())
    {
        buffer.resize(offset + data.per_read_size, '\0');
        socket.async_read_some(asio::buffer(buffer) += offset, cb);
        CORO_YIELD(state);

        if (_error)
        {
            SPDLOG_WARN("{} _error: {}", get_info(data), _error.message());
            return;
        }

        if (_bytes >= 0)
        {
            SPDLOG_INFO("{} _bytes: {}", get_info(data), _bytes);
            offset += _bytes;
        }
    }

    SPDLOG_INFO("{} : {}", get_info(data), buffer.data());

    offset = 0;
    buffer = to_buffer("server.");
    while (offset != buffer.size())
    {
        socket.async_write_some(asio::buffer(buffer) += offset, cb);
        CORO_YIELD(state);

        if (_error)
        {
            SPDLOG_WARN("{} _error: {}", get_info(data), _error.message());
            return;
        }

        if (_bytes >= 0)
        {
            offset += _bytes;
        }
    }

    SPDLOG_INFO("{} write fininish!", get_info(data));

    CORO_END();
}

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v");

    asio::io_context io_context;
    using asio::ip::tcp;

    acceptor_t acceptor(
        io_context,
        tcp::endpoint(tcp::v4(), 8888));

    auto data = std::make_shared<TcpConnection>(io_context);
    accept_socket(error_code_t{}, 0U, acceptor, data);

    io_context.run();

    return 0;
}
