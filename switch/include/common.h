#pragma once
#include <boost/system.hpp> // boost::system::error_code
#include <boost/asio/buffer.hpp>
#define BOOST_ASIO_NO_DEPRECATED 1
#include <boost/asio.hpp> // boost::asio
#include "log.hpp"

using error_code_t = boost::system::error_code;
namespace asio = boost::asio;

#include <cstddef>

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

#define CORO_YIELD_VALUE(_state, _value) \
    do                                   \
    {                                    \
        _state = __LINE__;               \
        return _value;                   \
    case __LINE__:;                      \
    } while (false)

#define CORO_END() }

using acceptor_t = asio::ip::tcp::acceptor;
using socket_t = asio::ip::tcp::socket;

struct Connection
{
    Connection(socket_t &&_socket) : socket(std::move(_socket)) {}
    ~Connection() = default;
    using buffer_t = std::vector<char>;
    buffer_t buffer;
    std::size_t offset = 0;
    socket_t socket;
    int state = 0;
};

using on_callback_t = void (*)(Connection& _data);

struct AcceptData
{
    AcceptData(acceptor_t &_acceptor) : acceptor(_acceptor) {}
    ~AcceptData() = default;
    int state = 0;
    acceptor_t &acceptor;
    std::shared_ptr<socket_t> socket;
};

using on_accept_t = void (*)(AcceptData& _data);

struct Accept
{
    Accept(acceptor_t &_acceptor, on_accept_t _on_accept)
        : m_data(std::make_shared<AcceptData>(_acceptor)), on_accept(_on_accept) {}
    ~Accept() = default;
    std::shared_ptr<AcceptData> m_data;
    on_accept_t on_accept;
    void operator()(error_code_t _error)
    {
        auto &data = *m_data;
        auto &state = data.state;
        auto &acceptor = data.acceptor;
        auto &socket = data.socket;
        if (_error)
        {
            SPDLOG_WARN(DBG(_error.message()));
            return;
        }

        CORO_BEGIN(state);
        while (true)
        {
            socket = std::make_shared<socket_t>(acceptor.get_executor());
            acceptor.async_accept(*socket, *this);
            CORO_YIELD(state);

            [&]
            {
                on_accept(data);
            }();
        }
        CORO_END();
    }
};

struct ConnectData
{
    ConnectData(
        asio::io_context &_io_context,
        asio::ip::tcp::endpoint &_endpoint)
        : endpoint(_endpoint), io_context(_io_context)
    {
        socket = std::make_shared<socket_t>(io_context);
    }
    ~ConnectData() = default;
    int state = 0;
    std::shared_ptr<socket_t> socket;
    asio::ip::tcp::endpoint &endpoint;
    asio::io_context &io_context;
};

using on_connect_t = void (*)(ConnectData& _data);

struct Connect
{
    Connect(
        asio::io_context &_io_context,
        asio::ip::tcp::endpoint &_endpoint,
        on_connect_t _on_connect)
        : m_data(std::make_shared<ConnectData>(_io_context, _endpoint)), on_connect(_on_connect) {}
    ~Connect() = default;
    std::shared_ptr<ConnectData> m_data;
    on_connect_t on_connect;
    void operator()(error_code_t _error)
    {
        auto &data = *m_data;
        auto &state = data.state;
        auto &socket = data.socket;
        auto &endpoint = data.endpoint;
        if (_error)
        {
            SPDLOG_WARN(DBG(_error.message()));
            return;
        }

        CORO_BEGIN(state);
        socket->async_connect(endpoint, *this);
        CORO_YIELD(state);

        [&]
        {
            on_connect(data);
        }();
    }
    CORO_END();
};
