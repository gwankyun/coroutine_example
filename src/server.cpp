module;
#include <memory> // std::shared_ptr std::make_shared
#include <string> // std::string
#include <vector> // std::vector
#include <coro.hpp>
module server;

using acceptor_t = asio::ip::tcp::acceptor;

void callback(std::shared_ptr<socket_t> _socket)
{
    auto vec = std::make_shared<std::vector<char>>(1024, '\0');
    _socket->async_read_some(
        asio::buffer(vec->data(), vec->size()),
        [=](error_code_t _error, std::size_t _bytes)
        {
            if (_error)
            {
                SPDLOG_DBG(INFO, _error.message());
                return;
            }
            SPDLOG_DBG(INFO, _bytes);

            SPDLOG_DBG(INFO, vec->data());

            auto str = std::make_shared<std::string>("server.");
            _socket->async_write_some(
                asio::buffer(str->c_str(), str->size()),
                [=](error_code_t _error, std::size_t _bytes)
                {
                    if (_error)
                    {
                        SPDLOG_DBG(INFO, _error.message());
                        return;
                    }
                    SPDLOG_DBG(INFO, _bytes);
                });
        });
}

lite::task<void> coro_resume(std::shared_ptr<socket_t> _socket)
{
    auto &socket = *_socket;
    using promise_type = lite::task<void>::promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle; // 用於保存協程句柄

    error_code_t error;
    std::size_t bytes;

    std::vector<char> vec(1024, '\0');
    co_await lite::async_resume(
        handle,
        [&]
        {
            socket.async_read_some(
                asio::buffer(vec.data(), vec.size()),
                [&](error_code_t _error, std::size_t _bytes)
                {
                    error = _error;
                    bytes = _bytes;
                    handle.resume();
                });
        });
    if (error)
    {
        SPDLOG_DBG(INFO, error.message());
        co_return;
    }
    SPDLOG_DBG(INFO, bytes);
    SPDLOG_DBG(INFO, vec.data());

    std::string str{"server."};
    co_await lite::async_resume(
        handle,
        [&]
        {
            socket.async_write_some(
                asio::buffer(str.c_str(), str.size()),
                [&](error_code_t _error, std::size_t _bytes)
                {
                    error = _error;
                    bytes = _bytes;
                    handle.resume();
                });
        });
    if (error)
    {
        SPDLOG_DBG(INFO, error.message());
        co_return;
    }
    SPDLOG_DBG(INFO, bytes);
}

template <typename T = void>
lite::task<T> coro_return(std::shared_ptr<socket_t> _socket)
{
    auto &socket = *_socket;
    using task_type = lite::task<T>;
    using promise_type = task_type::promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle; // 用於保存協程句柄

    error_code_t error;
    std::size_t bytes;

    std::vector<char> vec(1024, '\0');
    std::tie(error, bytes) = co_await lite::async_read(
        handle, socket, asio::buffer(vec.data(), vec.size()));
    if (error)
    {
        if constexpr (std::is_void_v<T>)
        {
            SPDLOG_DBG(INFO, error.message());
            co_return;
        }
        else
        {
            co_return error;
        }
    }
    SPDLOG_DBG(INFO, bytes);
    SPDLOG_DBG(INFO, vec.data());

    std::string str{"server."};
    std::tie(error, bytes) = co_await lite::async_write(
        handle, socket, asio::buffer(str.c_str(), str.size()));
    if (error)
    {
        if constexpr (std::is_void_v<T>)
        {
            SPDLOG_DBG(INFO, error.message());
            co_return;
        }
        else
        {
            co_return error;
        }
    }
    SPDLOG_DBG(INFO, bytes);

    if constexpr (std::is_void_v<T>)
    {
        SPDLOG_DBG(INFO, error.message());
        co_return;
    }
    else
    {
        co_return error;
    }
}

void on_accept(
    error_code_t _error,
    acceptor_t &_acceptor,
    std::shared_ptr<socket_t> _socket,
    AsyncType _type)
{
    if (_error)
    {
        SPDLOG_DBG(INFO, _error.message());
        return;
    }

    auto newSocket = std::make_shared<socket_t>(_acceptor.get_executor());
    _acceptor.async_accept(
        *newSocket,
        [=, &_acceptor](error_code_t error)
        {
            on_accept(error, _acceptor, newSocket, _type);
        });

    auto remote_endpoint = _socket->remote_endpoint();
    auto ip = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    SPDLOG_DBG(INFO, ip);
    SPDLOG_DBG(INFO, port);

    error_code_t error;

    switch (_type)
    {
    case AsyncType::Callback:
        callback(_socket); // 回調版本
        break;
    case AsyncType::CoroResume:
        coro_resume(_socket); // 協程包裹回調
        break;
    case AsyncType::CoroReturn:
        coro_return(_socket); // 協程封裝回調
        break;
    case AsyncType::CoroReturnValue:
    {
        error = coro_return<error_code_t>(_socket).get(); // 協程封裝回調返回值
        if (error)
        {
            SPDLOG_DBG(WARN, error.message());
        }
        break;
    }
    default:
        break;
    }
}

int main()
{
#if HAS_SPDLOG
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v");
#endif

    asio::io_context io_context;
    using asio::ip::tcp;

    acceptor_t acceptor(
        io_context,
        tcp::endpoint(tcp::v4(), 8888));

    {
        auto socket = std::make_shared<socket_t>(io_context);
        acceptor.async_accept(
            *socket,
            [=, &acceptor](error_code_t error)
            {
                on_accept(error, acceptor, socket, AsyncType::CoroReturnValue);
            });
    }

    io_context.run();

    return 0;
}
