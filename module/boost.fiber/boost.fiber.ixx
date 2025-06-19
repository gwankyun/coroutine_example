module;

// https://github.com/boostorg/fiber/issues/314
#define BOOST_FIBERS_STATIC_LINK
#include <boost/fiber/all.hpp>

export module boost.fiber;
export import boost.intrusive;

export namespace boost::fibers
{
    using boost::fibers::buffered_channel;
    using boost::fibers::channel_op_status;
    using boost::fibers::context;
    using boost::fibers::fiber;
} // namespace boost::fibers

export namespace boost::this_fiber
{
    using boost::this_fiber::yield;
} // namespace boost::this_fiber
