module;

export module boost.asio.example;
import std;
import boost.asio;

export int main(int _argc, char* _argv[])
{
    boost_asio::io_context context;
    boost_asio::post(context, [&] { std::println("yes"); });

    context.run();
    return 0;
}
