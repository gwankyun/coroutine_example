module;

export module asio_module_example;
import std;
import asio;

export int main(int _argc, char* _argv[])
{
    asio_module::io_context context;
    asio_module::post(context, [&] { std::println("yes"); });

    context.run();
    return 0;
}
