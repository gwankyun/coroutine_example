#include <boost/cobalt/main.hpp>
#include <boost/cobalt/op.hpp>
#include <spdlog/spdlog.h>
#include <boost/asio/steady_timer.hpp>

using namespace boost;

cobalt::main co_main(int argc, char* argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::steady_timer tim{co_await asio::this_coro::executor, std::chrono::milliseconds(std::stoi(argv[1]))};
    SPDLOG_INFO("before");
    co_await tim.async_wait(cobalt::use_op);
    SPDLOG_INFO("after");
    co_return 0;
}