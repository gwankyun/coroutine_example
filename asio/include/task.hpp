#include <concepts>   // std::invocable
#include <coroutine>  // std::coroutine_handle std::suspend_never
#include <functional> // std::function

#include <cstdlib> // std::exit

namespace type
{
    using coroutine = std::coroutine_handle<>;

    struct task
    {
        struct promise_type
        {
            task get_return_object()
            {
                return {};
            }
            std::suspend_never initial_suspend()
            {
                return {};
            }
            std::suspend_never final_suspend() noexcept
            {
                return {};
            }
            void unhandled_exception()
            {
                std::terminate();
            }
            void return_void()
            {
            }
        };
    };

    struct awaiter
    {
        using Fn = std::function<void(coroutine&)>;
        explicit awaiter(Fn _f) : f(_f)
        {
        }
        bool await_ready()
        {
            return false;
        }
        void await_suspend(coroutine _coroutine) // _coroutine為傳入的協程
        {
            f(_coroutine); // 暫停時調用
        }
        void await_resume()
        {
        }
        Fn f;
    };
} // namespace type

namespace func
{
    namespace t = type;
    /// @brief 在協程中處理回調
    /// @param _coroutine 協程
    /// @param _f 回調，在裡面執行_coroutine.resume()即恢復
    /// @return 可供co_await的協程
    t::awaiter async_resume(t::coroutine& _coroutine, std::function<void()> _f)
    {
        auto fn = [&, _f](const t::coroutine& _coro)
        {
            _coroutine = _coro; // 把協程句柄傳出去
            _f();               // 業務回調
        };
        return t::awaiter{fn};
    }
} // namespace func
