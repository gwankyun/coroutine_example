module;
#include <boost/core/ignore_unused.hpp>

#if !USE_BOOST_ASIO_MODULE
#  include <boost/asio.hpp>
#endif

export module boost.asio.example;
import std;

#if USE_BOOST_ASIO_MODULE
import boost.asio;
namespace asio = boost_asio;
#else
namespace asio = boost::asio;
#endif

export int main(int _argc, char* _argv[])
{
    boost::ignore_unused(_argc, _argv);
    asio::io_context context;
    asio::post(context, [&] { std::println("yes"); });

    context.run();
    return 0;
}
