module;
#include <boost/asio.hpp>

export module boost.asio;
import std.compat;

export namespace boost_asio_require_fn
{
    using boost_asio_require_fn::impl;
}

export namespace boost_asio_prefer_fn
{
    using boost_asio_prefer_fn::impl;
}

export namespace boost_asio
{
    namespace ip
    {
        using boost::asio::ip::tcp;
    }

    namespace execution
    {
        using boost::asio::execution::any_executor;
        namespace detail
        {
            using boost::asio::execution::detail::any_executor_base;
            using boost::asio::execution::detail::any_executor_context;
            //using boost::asio::execution::detail::find_context_as_property;
            using boost::asio::execution::detail::supportable_properties;
        }
    }

    namespace detail
    {
        using boost::asio::detail::initiate_post_with_executor;
    }

    //using boost::asio::execution;
    using boost::asio::io_context;
    //using boost::asio::post;
    using boost::asio::async_initiate;
    using boost::asio::default_completion_token_t;
    using boost::asio::constraint_t;
    using boost::asio::execution_context;

    auto post(io_context& _context, std::function<void()> _fn)
    {
        return boost::asio::post(_context, _fn);
    }
} // namespace boost_asio
