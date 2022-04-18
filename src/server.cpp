#include <coro.hpp>

namespace asio = boost::asio;
using error_code_t = boost::system::error_code;

void server_callback(std::shared_ptr<asio::ip::tcp::socket> _socket)
{
    auto vec = std::make_shared<std::vector<char>>(1024, '\0');
    _socket->async_read_some(
        asio::buffer(vec->data(), vec->size()),
        [_socket, vec](error_code_t _error, std::size_t _bytes)
        {
            if (_error)
            {
                SPDLOG_INFO(_error.message());
                return;
            }
            SPDLOG_INFO(_bytes);

            SPDLOG_INFO(vec->data());

            auto str = std::make_shared<std::string>("server.");
            _socket->async_write_some(
                asio::buffer(str->c_str(), str->size()),
                [_socket, str](error_code_t _error, std::size_t _bytes)
                {
                    if (_error)
                    {
                        SPDLOG_INFO(_error.message());
                        return;
                    }
                    SPDLOG_INFO(_bytes);
                });
        });
}

lite::task server_coro(std::shared_ptr<asio::ip::tcp::socket> _socket)
{
    auto& socket = *_socket;
    std::coroutine_handle<lite::task::promise_type> handle; // 用於保存協程句柄

    error_code_t error;
    std::size_t bytes;

    std::vector<char> vec(1024, '\0');
    co_await lite::await(
        handle,
        [&handle, &socket, &error, &bytes, &vec]()
        {
            socket.async_read_some(
                asio::buffer(vec.data(), vec.size()),
                [&handle, &error, &bytes](error_code_t _error, std::size_t _bytes)
                {
                    error = _error;
                    bytes = _bytes;
                    handle.resume();
                });
        });
    if (error)
    {
        SPDLOG_INFO(error.message());
        co_return;
    }
    SPDLOG_INFO(bytes);
    SPDLOG_INFO(vec.data());

    std::string str{ "server." };
    co_await lite::await(
        handle,
        [&handle, &socket, &error, &bytes, &str]()
        {
            socket.async_write_some(
                asio::buffer(str.c_str(), str.size()),
                [&handle, &error, &bytes](error_code_t _error, std::size_t _bytes)
                {
                    error = _error;
                    bytes = _bytes;
                    handle.resume();
                });
        });
    if (error)
    {
        SPDLOG_INFO(error.message());
        co_return;
    }
    SPDLOG_INFO(bytes);
    SPDLOG_INFO("write finished.");
}

lite::task server_coro_value(std::shared_ptr<asio::ip::tcp::socket> _socket)
{
    auto& socket = *_socket;
    std::coroutine_handle<lite::task::promise_type> handle; // 用於保存協程句柄

    std::vector<char> vec(1024, '\0');
    auto [error_r, bytes_r] = co_await lite::async_read(
        handle, socket, asio::buffer(vec.data(), vec.size()));
    if (error_r)
    {
        SPDLOG_INFO(error_r.message());
        co_return;
    }
    SPDLOG_INFO(bytes_r);
    SPDLOG_INFO(vec.data());

    std::string str{ "server." };
    auto [error_w, bytes_w] = co_await lite::async_write(
        handle, socket, asio::buffer(str.c_str(), str.size()));
    if (error_w)
    {
        SPDLOG_INFO(error_w.message());
        co_return;
    }
    SPDLOG_INFO(bytes_w);
    SPDLOG_INFO("write finished.");
}

lite::task_value<error_code_t> server_coro_return_value(std::shared_ptr<asio::ip::tcp::socket> _socket)
{
    auto& socket = *_socket;
    std::coroutine_handle<lite::task_value<error_code_t>::promise_type> handle; // 用於保存協程句柄

    error_code_t error;

    std::vector<char> vec(1024, '\0');
    auto [error_r, bytes_r] = co_await lite::async_read(
        handle, socket, asio::buffer(vec.data(), vec.size()));
    if (error_r)
    {
        SPDLOG_INFO(error_r.message());
        co_return error_r;
    }
    SPDLOG_INFO(bytes_r);
    SPDLOG_INFO(vec.data());

    std::string str{ "server." };
    auto [error_w, bytes_w] = co_await lite::async_write(
        handle, socket, asio::buffer(str.c_str(), str.size()));
    if (error_w)
    {
        SPDLOG_INFO(error_w.message());
        co_return error_w;
    }
    SPDLOG_INFO(bytes_w);
    SPDLOG_INFO("write finished.");

    co_return error;
}

void on_accept(
    error_code_t _error,
    asio::ip::tcp::acceptor& _acceptor,
    std::shared_ptr<asio::ip::tcp::socket> _socket)
{
    if (_error)
    {
        SPDLOG_INFO(_error.message());
        return;
    }

    auto newSocket = std::make_shared<asio::ip::tcp::socket>(_acceptor.get_executor());
    _acceptor.async_accept(
        *newSocket,
        [&_acceptor, newSocket](error_code_t error)
        {
            on_accept(error, _acceptor, newSocket);
        });

    auto remote_endpoint = _socket->remote_endpoint();
    SPDLOG_INFO(remote_endpoint.address().to_string());
    SPDLOG_INFO(remote_endpoint.port());

    //server_callback(_socket); // 回調版本
    //server_coro(_socket); // 協程包裹回調
    //server_coro_value(_socket); // 協程封裝回調
    auto error = server_coro_return_value(_socket).get();
    SPDLOG_INFO(error.message());
}

int main()
{
#if SPDLOG_EXISTS
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] [t:%6t] [p:%6P] [%-20!!:%4#] %v");
#endif

    asio::io_context io_context;

    asio::ip::tcp::acceptor acceptor(
        io_context,
        asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 8888));

    {
        auto socket = std::make_shared<asio::ip::tcp::socket>(io_context);
        acceptor.async_accept(
            *socket,
            [&acceptor, socket](error_code_t error)
            {
                on_accept(error, acceptor, socket);
            });
    }

    io_context.run();

    return 0;
}
