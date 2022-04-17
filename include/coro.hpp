#pragma once
#include <memory>
#include <string>
#include <vector>
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
        struct promise_type
        {
            task get_return_object() { return {}; }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception()
            {
                std::exit(1);
            }
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

    template<typename T, typename F>
    struct awaiter_value
    {
        awaiter_value(F _f) : f(_f) {}
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> _handle)
        {
            f(_handle, value);
        }
        T await_resume() { return value; }
        F f;
        T value{};
    };

    template<typename T, typename F>
    inline auto await_value(std::coroutine_handle<>& _handle, F _f)
    {
        auto fn = [&_handle, _f](std::coroutine_handle<> _hdl, T& _value)
        {
            _handle = _hdl;
            _f(_value);
        };
        return awaiter_value<T, decltype(fn)>{ fn };
    }

    inline auto async_read(
        std::shared_ptr<std::coroutine_handle<>> _handle,
        asio::ip::tcp::socket& _socket,
        asio::mutable_buffer _buffer)
    {
        using result_t = std::tuple<error_code_t, std::size_t>;
        return lite::await_value<result_t>(
            *_handle,
            [_handle, &_socket, _buffer](result_t& _result)
            {
                _socket.async_read_some(
                    _buffer,
                    [_handle, &_result](error_code_t _error, std::size_t _bytes)
                    {
                        _result = std::make_tuple(_error, _bytes);
                        _handle->resume();
                    });
            });
    }

    inline auto async_write(
        std::shared_ptr<std::coroutine_handle<>> _handle,
        asio::ip::tcp::socket& _socket,
        asio::const_buffer _buffer)
    {
        using result_t = std::tuple<error_code_t, std::size_t>;
        return lite::await_value<result_t>(
            *_handle,
            [_handle, &_socket, _buffer](result_t& _result)
            {
                _socket.async_write_some(
                    _buffer,
                    [_handle, &_result](error_code_t _error, std::size_t _bytes)
                    {
                        _result = std::make_tuple(_error, _bytes);
                        _handle->resume();
                    });
            });
    }
}
