#pragma once
#include <type_traits>
#include <tuple> // std::tuple
#define BOOST_ASIO_NO_DEPRECATED 1
#include <boost/asio.hpp>   // boost::asio
#include <boost/system.hpp> // boost::system::error_code
#include <coroutine>        // std::coroutine_handle std::suspend_never
#include <concepts>         // std::invocable

#ifndef HAS_SPDLOG
#define HAS_SPDLOG 0
#endif

#if HAS_SPDLOG
#include <spdlog/spdlog.h>
#else
#include <iostream>
#define SPDLOG_INFO(x) std::cout << (x) << "\n";
#endif

#ifndef TO_STRING_IMPL
#define TO_STRING_IMPL(x) #x
#endif

#ifndef TO_STRING
#define TO_STRING(x) TO_STRING_IMPL(x)
#endif

#ifndef CAT_IMPL
#define CAT_IMPL(a, b) a##b
#endif

#ifndef CAT
#define CAT(a, b) CAT_IMPL(a, b)
#endif

#define DBG(x) "{0}: {1}", TO_STRING(x), (x)

#define SPDLOG_DBG(_lvl, _value) CAT(SPDLOG_, _lvl)(DBG(_value))

namespace asio = boost::asio;
using error_code_t = boost::system::error_code;
using socket_t = asio::ip::tcp::socket;

enum struct AsyncType
{
    Callback = 0,
    CoroSwith,
    BoostCoro2,
    CoroResume,
    CoroResumeValue,
    CoroReturn,
    CoroReturnValue,
    Last
};

namespace lite
{
    struct task
    {
        struct promise_type; // std::coroutine_traits要求
        using handle_type = std::coroutine_handle<promise_type>;
        struct promise_type
        {
            task get_return_object()
            {
                return {handle_type::from_promise(*this)};
            }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception() { std::exit(1); }
        };
        handle_type handle; // 協程句柄
    };

    template <typename P, typename F>
        requires std::invocable<F, std::coroutine_handle<P> &>
    struct awaiter
    {
        using handle_type = std::coroutine_handle<P>;
        explicit awaiter(F _f) : f(_f) {}
        bool await_ready() { return false; }
        void await_suspend(handle_type _handle) // _handle為傳入的task::handle
        {
            f(_handle); // 暫停時調用
        }
        void await_resume() {}
        F f;
    };

    /// @brief 在協程中處理回調
    /// @param _handle 協程柄
    /// @param _f 回調
    /// @return 可供co_await的協程
    template <typename P, std::invocable F>
    inline auto async_resume(std::coroutine_handle<P> &_handle, F _f)
    {
        using handle_type = std::coroutine_handle<P>;
        auto fn = [&_handle, _f](const handle_type &_hdl)
        {
            _handle = _hdl; // 把協程句柄傳出去
            _f();           // 業務回調
        };
        return awaiter<P, decltype(fn)>{fn};
    }

    template <typename P, typename T, typename F>
        requires std::invocable<F, std::coroutine_handle<P> &, T &>
    struct awaiter_value
    {
        using handle_type = std::coroutine_handle<P>;
        explicit awaiter_value(F _f) : f(_f) {}
        bool await_ready() { return false; }
        void await_suspend(handle_type _handle)
        {
            f(_handle, value);
        }
        T await_resume() { return value; } // 這裡就是co_await的返回值
        F f;
        T value{}; // 用於保存co_await的返回值
    };

    /// @brief 在協程中處理回調並返回值
    /// @param _handle 協程柄
    /// @param _f 回調
    /// @return 可供co_await的協程
    template <typename T, typename P, typename F>
        requires std::invocable<F, T &>
    inline auto async_resume_value(std::coroutine_handle<P> &_handle, F _f)
    {
        using handle_type = std::coroutine_handle<P>;
        auto fn = [&_handle, _f](const handle_type &_hdl, T &_value)
        {
            _handle = _hdl;
            _f(_value);
        };
        return awaiter_value<P, T, decltype(fn)>{fn};
    }

    using result_type = std::tuple<error_code_t, std::size_t>;

    template <typename P>
    struct asio_callback
    {
        using handle_type = std::coroutine_handle<P>;
        asio_callback(handle_type &_handle, result_type &_result)
            : handle(_handle), result(_result) {}
        ~asio_callback() {}
        /// @brief 獲取回調的兩個值
        void operator()(error_code_t _error, std::size_t _bytes)
        {
            result = std::make_tuple(_error, _bytes); // 打包返回值
            handle.resume();                          // 恢復協程
        }

    private:
        result_type &result;
        handle_type &handle;
    };

    /// @brief 異步讀緩衝區
    /// @param _handle 協程柄
    /// @param _socket 連接
    /// @param _buffer 緩衝區
    /// @return [錯誤碼, 讀取長度]
    template <typename P, typename S>
    inline auto async_read(
        std::coroutine_handle<P> &_handle,
        S &_socket,
        asio::mutable_buffer _buffer)
    {
        auto fn = [&_handle, &_socket, _buffer](result_type &_result)
        {
            // 獲取回調兩個值並恢復
            _socket.async_read_some(_buffer, asio_callback(_handle, _result));
        };
        return async_resume_value<result_type, P>(_handle, fn);
    }

    /// @brief 異步寫緩衝區
    /// @param _handle 協程柄
    /// @param _socket 連接
    /// @param _buffer 緩衝區
    /// @return [錯誤碼, 寫入長度]
    template <typename P, typename S>
    inline auto async_write(
        std::coroutine_handle<P> &_handle,
        S &_socket,
        asio::const_buffer _buffer)
    {
        auto fn = [&_handle, &_socket, _buffer](result_type &_result)
        {
            // 獲取回調兩個值並恢復
            _socket.async_write_some(_buffer, asio_callback(_handle, _result));
        };
        return lite::async_resume_value<result_type, P>(_handle, fn);
    }

    template <typename T>
    struct task_value
    {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;
        struct promise_type
        {
            task_value get_return_object()
            {
                return {handle_type::from_promise(*this)};
            }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_value(T _value) { value = _value; } // 調用co_return
            void unhandled_exception()
            {
                std::exit(1);
            }
            T value{};
        };
        T get() { return handle.promise().value; }
        handle_type handle;
    };
}
