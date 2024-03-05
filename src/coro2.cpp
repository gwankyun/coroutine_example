// module;
#include <vector>
#include <string>
#include <spdlog/spdlog.h>
#define BOOST_ALL_NO_LIB 1
#include <boost/coroutine2/all.hpp>
// module coro2;
// import std.core;

namespace coro2 = boost::coroutines2;

void func(coro2::coroutine<void>::push_type &_yield)
{
    std::vector<int> vec{1, 2, 3};
    SPDLOG_INFO("{}", vec[0]);
    _yield();
    SPDLOG_INFO("{}", vec[1]);
    _yield();
    SPDLOG_INFO("{}", vec[2]);
}

void func2(coro2::coroutine<std::string>::push_type &_yield)
{
    SPDLOG_INFO("");
    _yield("1");
    SPDLOG_INFO("");
    _yield("2");
    SPDLOG_INFO("");
    _yield("3");
}

void example1()
{
    coro2::coroutine<void>::pull_type resume_func(func);

    std::vector<std::string> vec{"a", "b", "c"};

    SPDLOG_INFO("{}", vec[0]);
    resume_func();
    SPDLOG_INFO("{}", vec[1]);
    resume_func();
    SPDLOG_INFO("{}", vec[2]);
}

void example2()
{
    coro2::coroutine<std::string>::pull_type resume_func2(func2);

    while (resume_func2)
    {
        SPDLOG_INFO("{}", resume_func2.get());
        resume_func2();
        SPDLOG_INFO("");
    }
}

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-8!!:%4#] %v");

    // example1();

    example2();

    return 0;
}
