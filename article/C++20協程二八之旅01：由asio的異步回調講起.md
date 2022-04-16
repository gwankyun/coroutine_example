本篇默認讀者會用`asio`庫及掌握相關C++新特性。

傳統的回調：

``` C++
#include <memory>
#include <string>
#include <vector>
#define BOOST_ASIO_NO_DEPRECATED 1
#include <boost/asio.hpp>
#include <boost/system.hpp>
#include <spdlog/spdlog.h>
#include <coroutine> // 引入協程

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

    server_coro_value(_socket);
}

int main()
{
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] [t:%6t] [p:%6P] [%-20!!:%4#] %v");

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
```
上面是最簡單的例子，先讀再寫，回調嵌套回調，複雜點的邏輯很容易成為傳說中的「回調地獄」。

下面定義一些協程基礎設施：

``` C++
namespace lite
{
    struct task
    {
        struct promise_type
        {
            task get_return_object() { return {}; }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception() { std::exit(1); }
        };
    };

    template<typename F>
    struct awaiter
    {
        awaiter(F _f) : f(_f) {}
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> _handle) // _handle為傳入的協程句柄
        {
            f(_handle);
        }
        void await_resume() {}
        F f;
    };

    template<typename F>
    inline auto await(std::coroutine_handle<>& _handle, F _f)
    {
        return awaiter{
            [&_handle, _f](std::coroutine_handle<> _hdl)
            {
                _handle = _hdl; // 把協程句柄傳出去
                _f();
            }
        };
    }
}
```
上面提供了兩個類和一個函數，作用分別如下：

- `task`用作協程函數的返回值，所有協程函數都返回這個類型，當然這是個鴨子類型，不一定非得叫這個名，內容也可以按規則自定義。
- `awaiter`這個類型配合`co_await`操作符用於異步等待，這裡要注意`await_suspend`方法，它接受一個`std::coroutine_handle<> _handle`類型，這是操作協程的句柄，再通過回調函數`_f`傳出去。
- `await`是`awaiter`的封裝，把`await_suspend`參數拿到的協程句柄傳出去，回調函數`_f`負責執行業務邏輯。
  有了`task`和`await`，我們就可以使用協程封裝回調了：

``` C++
lite::task server_coro(std::shared_ptr<asio::ip::tcp::socket> _socket)
{
    auto& socket = *_socket;
    auto handle = std::make_shared<std::coroutine_handle<>>();

    error_code_t error;
    std::size_t bytes;

    std::vector<char> vec(1024, '\0');
    co_await lite::await(
        *handle,
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

    std::string str{ "server." };
    co_await lite::await(
        *handle,
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
}
```
正如上面所說的，`server_coro`的返回值是`task`。

然後用智能指針創建一個協程句柄`handle`，用於保存`awaiter::await_suspend`獲得的句柄，以便後續操作。

原先的回調調用都放在`co_await lite::await(...)`語句中，`vec`和`str`這些數據現在可以直接在棧上創建，回調裡面有一句`handle->resume();`十分關鍵，調用之後這個`co_await`語句會立即返回到調用處，達到恢復執行的效果。

相關代碼放在[gwankyun/coroutine_example: C++20協程示例](https://github.com/gwankyun/coroutine_example)，修改`server.cpp`中的`on_accept`函數及`client.cpp`中的`on_connect`函数中的相關行即可選擇使用回調還是協程。