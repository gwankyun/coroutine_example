module;
#include "coro.hpp"
#define BOOST_ALL_NO_LIB 1
#include <boost/coroutine2/all.hpp>
module client;

namespace coro2 = boost::coroutines2;

#define CORO_BEGIN(_state) \
    switch (_state)        \
    {                      \
    case 0:

#define CORO_RETURN(_state) \
    do                      \
    {                       \
        _state = __LINE__;  \
        return;             \
    case __LINE__:;         \
    } while (false)

#define CORO_END() }

asio::io_context *g_context = nullptr;

struct SwithContext
{
    SwithContext() : vec(1024, '\0') {}
    ~SwithContext() = default;
    std::string str{"client."};
    std::vector<char> vec;
    int state = 0;
};

void coro_switch(std::shared_ptr<SwithContext> _context, std::shared_ptr<socket_t> _socket, error_code_t _error, std::size_t _bytes)
{
    auto &context = *_context;
    auto &socket = *_socket;
    auto &state = context.state;
    auto &str = context.str;
    auto &vec = context.vec;
    CORO_BEGIN(state);
    // send
    socket.async_write_some(
        asio::buffer(str.c_str(), str.size()),
        [=](error_code_t _error, std::size_t _bytes)
        {
            coro_switch(_context, _socket, _error, _bytes); // 回調本函数
        });
    CORO_RETURN(state); // 返回但下次直接跳轉到此處
    if (_error)
    {
        SPDLOG_DBG(INFO, _error.message());
        return;
    }
    else
    {
        SPDLOG_INFO("send success!");
    }

    // recv
    socket.async_read_some(
        asio::buffer(vec.data(), vec.size()),
        [=](error_code_t _error, std::size_t _bytes)
        {
            coro_switch(_context, _socket, _error, _bytes); // 回調本函数
        });
    CORO_RETURN(state); // 返回但下次直接跳轉到此處
    if (_error)
    {
        SPDLOG_DBG(INFO, _error.message());
        return;
    }
    else
    {
        SPDLOG_INFO("recv success!");
    }

    SPDLOG_DBG(INFO, _bytes);
    SPDLOG_DBG(INFO, vec.data());
    CORO_END();
}

coro2::coroutine<void>::push_type *g_yield = nullptr;

struct Coro2Yield
{
    Coro2Yield(asio::io_context &_io_context) : m_io_context(_io_context) {}
    ~Coro2Yield() = default;

    void operator()(coro2::coroutine<void>::push_type &_yield)
    {
        g_yield = &_yield;
        while (true)
        {
            _yield();           // 回到主协程創建任務
            m_io_context.run(); // 从主协程回来，開始跑任務
        }
    }

private:
    asio::io_context &m_io_context;
};

void coro2_main(asio::io_context &_io_context, socket_t &_socket)
{
    Coro2Yield coro2_yield(_io_context);
    coro2::coroutine<void>::pull_type resume(coro2_yield);
    error_code_t error;
    std::size_t bytes;

    // send
    std::string str{"client."};
    _socket.async_write_some(
        asio::buffer(str.c_str(), str.size()),
        [&](error_code_t _error, std::size_t _bytes)
        {
            error = _error;
            bytes = _bytes;
            if (g_yield != nullptr)
            {
                (*g_yield)(); // 執行完回調時跳回主协程
            }
        });
    resume(); // 跳到子協程
    if (error)
    {
        SPDLOG_DBG(INFO, error.message());
        return;
    }
    else
    {
        SPDLOG_INFO("send success!");
    }

    // recv
    std::vector<char> vec(1024, '\0');
    _socket.async_read_some(
        asio::buffer(vec.data(), vec.size()),
        [&](error_code_t _error, std::size_t _bytes)
        {
            error = _error;
            bytes = _bytes;
            if (g_yield != nullptr)
            {
                (*g_yield)(); // 執行完回調時跳回主协程
            }
        });
    resume(); // 跳到子協程
    if (error)
    {
        SPDLOG_DBG(INFO, error.message());
        return;
    }
    else
    {
        SPDLOG_INFO("recv success!");
    }

    SPDLOG_DBG(INFO, bytes);
    SPDLOG_DBG(INFO, vec.data());
}

void callback(std::shared_ptr<socket_t> _socket)
{
    auto str = std::make_shared<std::string>("client.");
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
                });
        });
}

lite::task<error_code_t> coro_resume(std::shared_ptr<socket_t> _socket)
{
    auto &socket = *_socket;
    using task_type = lite::task<error_code_t>;
    using promise_type = task_type::promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle; // 用於保存協程句柄

    error_code_t error;
    std::size_t bytes;

    std::string str{"client."};
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
                    handle.resume(); // |
                });                  // |
        });                          // v
    // <---------------------------------
    if (error)
    {
        co_return error;
    }
    SPDLOG_DBG(INFO, bytes);

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
                    handle.resume(); // |
                });                  // |
        });                          // v
    // <---------------------------------
    if (error)
    {
        co_return error;
    }
    SPDLOG_DBG(INFO, bytes);
    SPDLOG_DBG(INFO, vec.data());
    co_return error;
}

lite::task<error_code_t> coro_resume_value(std::shared_ptr<socket_t> _socket)
{
    auto &socket = *_socket;
    using task_type = lite::task<error_code_t>;
    using promise_type = task_type::promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle; // 用於保存協程句柄

    using result_type = std::tuple<error_code_t, std::size_t>;

    error_code_t error;
    std::size_t bytes;

    std::string str{"client."};
    std::tie(error, bytes) = co_await lite::async_resume_value<result_type>(
        handle,
        [&](result_type &_result)
        {
            socket.async_write_some(
                asio::buffer(str.c_str(), str.size()),
                [&](error_code_t _error, std::size_t _bytes)
                {
                    _result = std::make_tuple(_error, _bytes);
                    handle.resume(); // |
                });                  // |
        });                          // v
    // <---------------------------------
    if (error)
    {
        co_return error;
    }
    SPDLOG_DBG(INFO, bytes);

    std::vector<char> vec(1024, '\0');
    std::tie(error, bytes) = co_await lite::async_resume_value<result_type>(
        handle,
        [&](result_type &_result)
        {
            socket.async_read_some(
                asio::buffer(vec.data(), vec.size()),
                [&](error_code_t _error, std::size_t _bytes)
                {
                    _result = std::make_tuple(_error, _bytes);
                    handle.resume(); // |
                });                  // |
        });                          // v
    // <---------------------------------
    if (error)
    {
        co_return error;
    }
    SPDLOG_DBG(INFO, bytes);
    SPDLOG_DBG(INFO, vec.data());

    if (error)
    {
        co_return error;
    }
}

template<typename T = void>
lite::task<T> coro_return(std::shared_ptr<socket_t> _socket)
{
    auto &socket = *_socket;
    using task_type = lite::task<T>;
    using promise_type = task_type::promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle; //

    error_code_t error;
    std::size_t bytes;

    std::string str{"client."};
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

void on_connect(
    asio::io_context &_io_context,
    error_code_t _error,
    std::shared_ptr<socket_t> _socket,
    AsyncType _type)
{
    if (_error)
    {
        SPDLOG_DBG(INFO, _error.message());
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
        error = coro_resume(_socket).get(); // 協程包裹回調
        if (error)
        {
            SPDLOG_DBG(WARN, error.message());
        }
        break;
    case AsyncType::CoroResumeValue:
        error = coro_resume_value(_socket).get();
        if (error)
        {
            SPDLOG_DBG(WARN, error.message());
        }
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
    case AsyncType::CoroSwith: // switch無棧協程
    {
        auto state = std::make_shared<SwithContext>();
        coro_switch(state, _socket, error, 0U);
        break;
    }
    case AsyncType::BoostCoro2: // boost有棧协程
    {
        coro2_main(_io_context, *_socket);
        break;
    }
    default:
        break;
    }
}

int main(int _argc, char *_argv[])
{
#if HAS_SPDLOG
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v");
#endif

    SPDLOG_DBG(INFO, _argc);

    auto asyncType = AsyncType::CoroSwith; // 默認版本
    if (_argc >= 2)
    {
        auto type = std::stoi(_argv[1]);
        if (0 <= type && type < int(AsyncType::Last))
        {
            asyncType = AsyncType(type);
        }
    }

    asio::io_context io_context;
    namespace ip = asio::ip;

    {
        auto socket = std::make_shared<socket_t>(io_context);
        auto address = ip::make_address("127.0.0.1");
        socket->async_connect(
            ip::tcp::endpoint(address, 8888),
            [=, &io_context](error_code_t _error)
            {
                on_connect(io_context, _error, socket, asyncType);
            });
    }

    io_context.run();

    return 0;
}
