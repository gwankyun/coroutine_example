module;
//#include <boost/coroutine2/all.hpp>
#include <boost/context/continuation.hpp>
#include <boost/context/fiber.hpp>

export module boost.context;

export namespace boost::context
{
    //using boost::coroutines2::coroutine;
    using boost::context::continuation;
    using boost::context::callcc;
    using boost::context::fiber;
}
