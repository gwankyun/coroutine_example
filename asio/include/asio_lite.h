#pragma once

#include <functional>
#include <queue>

namespace lite
{
    namespace asio
    {
        using Fn = std::function<void()>;

        struct io_context
        {
            io_context();
            ~io_context();

            void run();

            std::queue<Fn> fn_queue;
        };

        void post(io_context& _context, Fn _f);
    } // namespace asio
} // namespace lite
