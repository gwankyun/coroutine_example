#include <coroutine>
#include <iostream>
#include <thread>

std::coroutine_handle<> handle;

struct ReadAwaiter
{
    bool await_ready()
    {
        std::cout << "current, no data to read" << std::endl;
        return false;
    }

    void await_resume()
    {
        std::cout << "get data to read" << std::endl;
    }

    void await_suspend(std::coroutine_handle<> h)
    {
        std::cout << "suspended self, wait data to read" << std::endl;
        handle = h;
    }
};

struct Promise
{
    struct promise_type
    {
        auto get_return_object() noexcept
        {
            std::cout << "get return object" << std::endl;
            return Promise();
        }

        auto initial_suspend() noexcept
        {
            std::cout << "initial suspend, return never" << std::endl;
            return std::suspend_never{};
        }

        auto final_suspend() noexcept
        {
            std::cout << "final suspend, return never" << std::endl;
            return std::suspend_never{};
        }

        void unhandled_exception()
        {
            std::cout << "unhandle exception" << std::endl;
            std::terminate();
        }

        void return_void()
        {
            std::cout << "return void" << std::endl;
            return;
        }
    };
};

Promise ReadCoroutineFunc()
{
    co_await ReadAwaiter();
}

int main()
{
    ReadCoroutineFunc();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "sleep 1s and then read data" << std::endl;
    handle.resume();
}
