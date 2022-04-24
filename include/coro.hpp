#pragma once
#include <memory>
#include <string>
#include <vector>
#include <type_traits>
#include <tuple>
#define BOOST_ASIO_NO_DEPRECATED 1
#include <boost/asio.hpp>
#include <boost/system.hpp>
#include <coroutine> // 引入協程

#ifndef SPDLOG_EXISTS
#  define SPDLOG_EXISTS 0
#endif

#if SPDLOG_EXISTS
#  include <spdlog/spdlog.h>
#else
#  include <iostream>
#  define SPDLOG_INFO(x) std::cout << (x) << "\n";
#endif

namespace asio = boost::asio;
using error_code_t = boost::system::error_code;

namespace lite
{
    struct task
    {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;
        struct promise_type
        {
            task get_return_object()
            {
                return { handle_type::from_promise(*this) };
            }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception() { std::exit(1); }
        };
        handle_type handle; // 協程句柄
    };

    template<typename P, typename F>
        requires requires (F _f, std::coroutine_handle<P> _handle) { _f(_handle); }
    struct awaiter
    {
        awaiter(F _f) : f(_f) {}
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<P> _handle) // _handle為傳入的task::handle
        {
            f(_handle);
        }
        void await_resume() {}
        F f;
    };

    template<typename P, typename F>
        requires requires (F _f) { _f(); }
    inline auto await(std::coroutine_handle<P>& _handle, F _f)
    {
        auto fn = [&_handle, _f](std::coroutine_handle<P>& _hdl)
        {
            _handle = _hdl; // 把協程句柄傳出去
            _f();
        };
        return awaiter<P, decltype(fn)>{ fn };
    }

    template<typename P, typename T, typename F>
        requires requires (F _f, std::coroutine_handle<P> _handle, T& _value) { _f(_handle, _value); }
    struct awaiter_value
    {
        awaiter_value(F _f) : f(_f) {}
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<P> _handle)
        {
            f(_handle, value);
        }
        T await_resume() { return value; } // 這裡就是co_await的返回值
        F f;
        T value{}; // 用於保存co_await的返回值
    };

    template<typename P, typename T, typename F>
        requires requires (F _f, T& _value) { _f(_value); }
    inline auto await_value(std::coroutine_handle<P>& _handle, F _f)
    {
        auto fn = [&_handle, _f](std::coroutine_handle<P>& _hdl, T& _value)
        {
            _handle = _hdl;
            _f(_value);
        };
        return awaiter_value<P, T, decltype(fn)>{ fn };
    }

    template<typename P>
    inline auto async_read(
        std::coroutine_handle<P>& _handle,
        asio::ip::tcp::socket& _socket,
        asio::mutable_buffer _buffer)
    {
        using result_t = std::tuple<error_code_t, std::size_t>;
        return lite::await_value<P, result_t>(
            _handle,
            [&_handle, &_socket, _buffer](result_t& _result)
            {
                _socket.async_read_some(
                    _buffer,
                    [&_handle, &_result](error_code_t _error, std::size_t _bytes)
                    {
                        _result = std::make_tuple(_error, _bytes);
                        _handle.resume();
                    });
            });
    }

    template<typename P>
    inline auto async_write(
        std::coroutine_handle<P>& _handle,
        asio::ip::tcp::socket& _socket,
        asio::const_buffer _buffer)
    {
        using result_t = std::tuple<error_code_t, std::size_t>;
        return lite::await_value<P, result_t>(
            _handle,
            [&_handle, &_socket, _buffer](result_t& _result)
            {
                _socket.async_write_some(
                    _buffer,
                    [&_handle, &_result](error_code_t _error, std::size_t _bytes)
                    {
                        _result = std::make_tuple(_error, _bytes);
                        _handle.resume();
                    });
            });
    }

    template<typename T>
    struct task_value
    {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;
        struct promise_type
        {
            task_value get_return_object() 
            {
                return { handle_type::from_promise(*this) };
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
