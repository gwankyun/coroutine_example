#include "coro.hpp"
#include <asio.hpp>
#include <iostream>

lite::task<int> task(asio::io_context& io_context, int x)
{
    using task_type = lite::task<int>;
    using promise_type = task_type::promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle; // 用於保存協程句柄

    auto result = co_await lite::async_resume_value<int, promise_type>(handle,
                                                         [&](int _v)
                                                         {
                                                             asio::post(io_context,
                                                                        [&]
                                                                        {
                                                                            _v = x;
                                                                            handle.resume();
                                                                        });
                                                         });
    co_return result;
}

int main()
{
    asio::io_context io_context;

    asio::post(io_context,
               [&]
               {
                   auto x = task(io_context, 1).get();

                   std::cout << x << std::endl;
               });

    io_context.run();

    return 0;
}
