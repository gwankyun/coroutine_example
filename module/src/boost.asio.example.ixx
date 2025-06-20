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
#pragma message("enable boost.asio module")
#else
namespace asio = boost::asio;
#pragma message("disable boost.asio module")
#endif

export int main(int _argc, char* _argv[])
{
    boost::ignore_unused(_argc, _argv);
    asio::io_context context;

    //asio::ip::tcp::acceptor acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 8888));

    //asio::ip::tcp::acceptor* acceptor = nullptr;

    asio::post(context, [&] { std::println("yes"); });

    context.run();
    return 0;
}
