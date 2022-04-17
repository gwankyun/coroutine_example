#include <coro.hpp>

void client_callback(std::shared_ptr<asio::ip::tcp::socket> _socket)
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
        }
    );
}

lite::task client_coro(std::shared_ptr<asio::ip::tcp::socket> _socket)
{
    auto& socket = *_socket;
    auto handle = std::make_shared<std::coroutine_handle<>>();

    error_code_t error;
    std::size_t bytes;

    std::string str{ "client." };
    co_await lite::await(*handle,
        [handle, &socket, &error, &bytes, &str]()
        {
            socket.async_write_some(
                asio::buffer(str.c_str(), str.size()),
                [handle, &error, &bytes](error_code_t _error, std::size_t _bytes)
                {
                    error = _error;
                    bytes = _bytes;
                    handle->resume();
                });
        });
    if (error)
    {
        SPDLOG_INFO(error.message());
        co_return;
    }
    SPDLOG_INFO(bytes);
    SPDLOG_INFO("write finished.");

    std::vector<char> vec(1024, '\0');
    co_await lite::await(*handle,
        [handle, &socket, &error, &bytes, &vec]()
        {
            socket.async_read_some(
                asio::buffer(vec.data(), vec.size()),
                [handle, &error, &bytes](error_code_t _error, std::size_t _bytes)
                {
                    error = _error;
                    bytes = _bytes;
                    handle->resume();
                });
        });
    if (error)
    {
        SPDLOG_INFO(error.message());
        co_return;
    }
    SPDLOG_INFO(bytes);
    SPDLOG_INFO(vec.data());
}

lite::task client_coro_value(std::shared_ptr<asio::ip::tcp::socket> _socket)
{
    auto& socket = *_socket;
    auto handle = std::make_shared<std::coroutine_handle<>>();

    std::string str{ "client." };
    auto [error_w, bytes_w] = co_await lite::async_write(
        handle, socket, asio::buffer(str.c_str(), str.size()));
    if (error_w)
    {
        SPDLOG_INFO(error_w.message());
        co_return;
    }
    SPDLOG_INFO(bytes_w);
    SPDLOG_INFO("write finished.");

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
}

void on_connect(
    error_code_t _error,
    std::shared_ptr<asio::ip::tcp::socket> _socket)
{
    if (_error)
    {
        SPDLOG_INFO(_error.message());
        return;
    }

    auto remote_endpoint = _socket->remote_endpoint();
    SPDLOG_INFO(remote_endpoint.address().to_string());
    SPDLOG_INFO(remote_endpoint.port());

    //client_callback(socket); // 回調版本
    //client_coro(_socket); // 協程包裹回調
    client_coro_value(_socket); // 協程封裝回調
}

int main()
{
#if SPDLOG_EXISTS
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] [t:%6t] [p:%6P] [%-20!!:%4#] %v");
#endif

    asio::io_context io_context;

    {
        auto socket = std::make_shared<asio::ip::tcp::socket>(io_context);
        socket->async_connect(
            asio::ip::tcp::endpoint(
                asio::ip::make_address("127.0.0.1"),
                8888),
            [socket](error_code_t _error)
            {
                on_connect(_error, socket);
            });
    }

    io_context.run();

    return 0;
}
