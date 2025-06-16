module;
//#include <boost/coroutine2/all.hpp>
//#include <boost/context/continuation.hpp>
//#include <boost/context/fiber.hpp>

// https://github.com/boostorg/fiber/issues/314
#define BOOST_FIBERS_STATIC_LINK
#include <boost/fiber/all.hpp>

export module boost.fiber;

export namespace boost::fibers
{
    //using boost::coroutines2::coroutine;
    //using boost::context::continuation;
    //using boost::context::callcc;
    //using boost::context::fiber;

    using boost::fibers::buffered_channel;
    using boost::fibers::channel_op_status;
    using boost::fibers::fiber;
}

export namespace boost::this_fiber
{
    using boost::this_fiber::yield;
}
