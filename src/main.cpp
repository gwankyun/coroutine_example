#include <spdlog/spdlog.h>

#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#define CORO_BEGIN(_state) \
    switch (_state) \
    { \
    case 0:

#define CORO_YIELD(_state) \
    do \
    { \
        _state = __LINE__; \
        return; \
    case __LINE__:; \
    } while (false)

#define CORO_END() }

void func(int& _state, int& _i, std::string& _output)
{
    /*
    switch (_state)
    {
    case 0:
    */
    CORO_BEGIN(_state);
    // 開始
    for (_i = 0; _i < 10; ++_i)
    {
        /*
        do
        {
            _state = 1; // 會回到case 1
            return _i;
        case 1:; // 恢復控制
        } while (false);
        */
        _output += std::to_string(_i);
        SPDLOG_INFO("{}", _i);
        CORO_YIELD(_state);
    }
    /*
    }
    */
    CORO_END();
}

enum class Flag
{
    begin,
    fun1,
    fun2,
    end
};

void fun1(int& _state, Flag& _flag, std::size_t& _offset, std::string& _output)
{
    std::string str = "123";

    CORO_BEGIN(_state);

    SPDLOG_INFO("wait");
    while (_flag != Flag::fun1)
    {
        CORO_YIELD(_state);
    }
    SPDLOG_INFO("run");
    _flag = Flag::fun2;
    SPDLOG_INFO("call fun2");
    CORO_YIELD(_state);

    for (_offset = 0; _offset < str.size(); _offset++)
    {
        SPDLOG_INFO(str[_offset]);
        _output += str[_offset];
        CORO_YIELD(_state);
    }

    CORO_END();
}

void fun2(int& _state, Flag& _flag, std::size_t& _offset, std::string& _output)
{
    std::string str = "abc";

    CORO_BEGIN(_state);

    SPDLOG_INFO("call fun1");
    _flag = Flag::fun1;
    SPDLOG_INFO("wait");
    while (_flag != Flag::fun2)
    {
        CORO_YIELD(_state);
    }
    SPDLOG_INFO("run");

    for (_offset = 0; _offset < str.size(); _offset++)
    {
        SPDLOG_INFO(str[_offset]);
        _output += str[_offset];
        CORO_YIELD(_state);
    }

    _flag = Flag::end;

    CORO_END();
}

TEST_CASE("switch coro", "[single]")
{
    int state = 0;
    int value = 0;
    std::string result;
    for (auto i = 0U; i < 10; ++i)
    {
        func(state, value, result);
    }
    REQUIRE(result == "0123456789");
}

TEST_CASE("switch coro", "[multiple]")
{
    int state1 = 0;
    int state2 = 0;
    std::size_t offset1 = 0u;
    std::size_t offset2 = 0u;
    Flag flag = Flag::begin;
    std::string result;
    while (flag != Flag::end)
    {
        fun1(state1, flag, offset1, result);
        fun2(state2, flag, offset2, result);
    }
    REQUIRE(result == "a1b2c3");
}

int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
