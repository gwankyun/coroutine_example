#include <coro.hpp>

void callback(std::shared_ptr<socket_t> _socket)
{
    auto str = std::make_shared<std::string>("client.");
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
                });
        });
}

lite::task coro_resume(std::shared_ptr<socket_t> _socket)
{
    auto &socket = *_socket;
    using promise_type = lite::task::promise_type;
    std::coroutine_handle<promise_type> handle; // 用於保存協程句柄

    error_code_t error;
    std::size_t bytes;

    std::string str{"client."};
    co_await lite::await(
        handle,
        [&handle, &socket, &error, &bytes, &str]
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
        SPDLOG_INFO(DBG(error.message()));
        co_return;
    }
    SPDLOG_INFO(DBG(bytes));

    std::vector<char> vec(1024, '\0');
    co_await lite::await(
        handle,
        [&handle, &socket, &error, &bytes, &vec]
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
        SPDLOG_INFO(DBG(error.message()));
        co_return;
    }
    SPDLOG_INFO(DBG(bytes));
    SPDLOG_INFO(DBG(vec.data()));
}

lite::task coro_return(std::shared_ptr<socket_t> _socket)
{
    auto &socket = *_socket;
    using promise_type = lite::task::promise_type;
    std::coroutine_handle<promise_type> handle; // 用於保存協程句柄

    std::string str{"client."};
    {
        auto [error, bytes] = co_await lite::async_write(
            handle, socket, asio::buffer(str.c_str(), str.size()));
        if (error)
        {
            SPDLOG_INFO(DBG(error.message()));
            co_return;
        }
        SPDLOG_INFO(DBG(bytes));
    }

    std::vector<char> vec(1024, '\0');
    {
        auto [error, bytes] = co_await lite::async_read(
            handle, socket, asio::buffer(vec.data(), vec.size()));
        if (error)
        {
            SPDLOG_INFO(DBG(error.message()));
            co_return;
        }
        SPDLOG_INFO(DBG(bytes));
    }
    SPDLOG_INFO(DBG(vec.data()));
}

lite::task_value<error_code_t> coro_return_value(std::shared_ptr<socket_t> _socket)
{
    auto &socket = *_socket;
    using promise_type = lite::task_value<error_code_t>::promise_type;
    std::coroutine_handle<promise_type> handle; // 用於保存協程句柄

    error_code_t error;

    std::string str{"client."};
    {
        auto [error, bytes] = co_await lite::async_write(
            handle, socket, asio::buffer(str.c_str(), str.size()));
        if (error)
        {
            SPDLOG_INFO(DBG(error.message()));
            co_return error;
        }
        SPDLOG_INFO(DBG(bytes));
    }

    std::vector<char> vec(1024, '\0');
    {
        auto [error, bytes] = co_await lite::async_read(
            handle, socket, asio::buffer(vec.data(), vec.size()));
        if (error)
        {
            SPDLOG_INFO(DBG(error.message()));
            co_return error;
        }
        SPDLOG_INFO(DBG(bytes));
    }
    SPDLOG_INFO(DBG(vec.data()));

    co_return error;
}

void on_connect(
    error_code_t _error,
    std::shared_ptr<socket_t> _socket,
    AsyncType _type)
{
    if (_error)
    {
        SPDLOG_INFO(DBG(_error.message()));
        return;
    }

    auto remote_endpoint = _socket->remote_endpoint();
    SPDLOG_INFO("ip: {}", remote_endpoint.address().to_string());
    SPDLOG_INFO("port: {}", remote_endpoint.port());
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
        error = coro_return_value(_socket).get(); // 協程封裝回調返回值
        SPDLOG_WARN(error.message());
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

    {
        auto socket = std::make_shared<socket_t>(io_context);
        socket->async_connect(
            asio::ip::tcp::endpoint(
                asio::ip::make_address("127.0.0.1"),
                8888),
            [socket](error_code_t _error)
            {
                on_connect(_error, socket, AsyncType::CoroReturnValue);
            });
    }

    io_context.run();

    return 0;
}
