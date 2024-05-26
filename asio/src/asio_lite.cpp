#include "../include/asio_lite.h"

namespace lite
{
    namespace asio
    {
        io_context::io_context()
        {
        }

        io_context::~io_context()
        {
        }

        void io_context::run()
        {
            while (!fn_queue.empty())
            {
                auto& head = fn_queue.front();
                head();
                fn_queue.pop();
            }
        }

        void post(io_context& _context, Fn _f)
        {
            _context.fn_queue.push(_f);
        }
    } // namespace asio
} // namespace lite
