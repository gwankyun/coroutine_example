#pragma once

#include <functional>
#include <queue>

// clang-format off
#ifndef ASIO_LITE_HEADR_ONLY
#  define ASIO_LITE_HEADR_ONLY 1
#endif
// clang-format on

namespace lite
{
    namespace asio
    {
        using Fn = std::function<void()>;

        namespace detail
        {
            struct io_context
            {
                io_context() = default;
                ~io_context() = default;

                void run()
                {
                    while (!fn_queue.empty())
                    {
                        auto& head = fn_queue.front();
                        head();
                        fn_queue.pop();
                    }
                }

                std::queue<Fn> fn_queue;
            };

            void post(io_context& _context, Fn _f)
            {
                _context.fn_queue.push(_f);
            }
        } // namespace detail

#if ASIO_LITE_HEADR_ONLY
        using detail::io_context;
#else
        struct io_context
        {
            io_context() = default;
            ~io_context() = default;

            void run();

            std::queue<Fn> fn_queue;
        };

        void post(io_context& _context, Fn _f);
#endif
    } // namespace asio
} // namespace lite
