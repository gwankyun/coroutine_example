module;
#include <boost/asio.hpp>

export module boost.asio;
import std.compat;

export namespace boost_asio
{
    using boost::asio::io_context;

    auto post(io_context& _context, std::function<void()> _fn)
    {
        return boost::asio::post(_context, _fn);
    }
} // namespace boost_asio
