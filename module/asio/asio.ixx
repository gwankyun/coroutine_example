module;
#include <asio.hpp>

export module asio;
import std;

export namespace asio_module
{
    struct Context
    {
        asio::io_context data;
        auto run()
        {
            return data.run();
        }
    };

    using io_context = Context;

    auto post(io_context& _context, std::function<void()> _fn)
    {
        return asio::post(_context.data, _fn);
    }
} // namespace asio_module
