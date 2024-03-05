#include <spdlog/spdlog.h>

#define CORO_BEGIN(_state) \
    switch (_state)        \
    {                      \
    case 0:

#define CORO_RETURN_VALUE(_state, _value) \
    do                                    \
    {                                     \
        _state = __LINE__;                \
        return _value;                    \
    case __LINE__:;                       \
    } while (false)

#define CORO_RETURN(_state) \
    do                      \
    {                       \
        _state = __LINE__;  \
        return;             \
    case __LINE__:;         \
    } while (false)

#define CORO_END() }

// clang-format off
#pragma warning(push)
#  pragma warning(disable: 4715)
// clang-format of

int func(int &_state, int &_i)
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
        CORO_RETURN_VALUE(_state, _i);
    }
    /*
    }
    */
    CORO_END();
}

int fun1(int& _state, int& _flag)
{
    CORO_BEGIN(_state);
    while (true)
    {
        SPDLOG_INFO("wait");
        if (_flag != 1)
        {
            CORO_RETURN_VALUE(_state, 0);
        }
        SPDLOG_INFO("run");
        _flag = 2;
        SPDLOG_INFO("call fun2");
        CORO_RETURN_VALUE(_state, 0);

        SPDLOG_INFO("1");
        CORO_RETURN_VALUE(_state, 0);
        SPDLOG_INFO("2");
        CORO_RETURN_VALUE(_state, 0);
        SPDLOG_INFO("3");
        CORO_RETURN_VALUE(_state, 0);
    }
    CORO_END();
}

int fun2(int& _state, int& _flag)
{
    CORO_BEGIN(_state);
    while (true)
    {
        SPDLOG_INFO("call fun1");
        _flag = 1;
        SPDLOG_INFO("wait");
        if (_flag != 2)
        {
            CORO_RETURN_VALUE(_state, 0);
        }
        SPDLOG_INFO("run");

        SPDLOG_INFO("a");
        CORO_RETURN_VALUE(_state, 0);
        SPDLOG_INFO("b");
        CORO_RETURN_VALUE(_state, 0);
        SPDLOG_INFO("c");
        CORO_RETURN_VALUE(_state, 0);

        _flag = 3;
        CORO_RETURN_VALUE(_state, 0);
    }
    CORO_END();
}

#pragma warning(pop)

int main()
{
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] [t:%6t] [p:%6P] [%-20!!:%4#] %v");

    int state = 0;
    int value = 0;
    for (auto i = 0U; i < 10; ++i)
    {
        SPDLOG_INFO("{}", func(state, value));
    }

    {
        int state1 = 0;
        int state2 = 0;
        int flag = 0;
        while (flag != 3)
        {
            fun1(state1, flag);
            fun2(state2, flag);
        }
    }

    return 0;
}
