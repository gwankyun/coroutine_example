#include <map>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/mdc.h>
#include <string>
#include <vector>
#include <queue>
#include <cassert>
#define BOOST_ALL_NO_LIB 1
#include <boost/context/continuation.hpp>

namespace context = boost::context;
using continuation_t = context::continuation;

void single_continuation()
{
    continuation_t source = context::callcc(
        [](continuation_t&& _sink)
        {
            _sink = _sink.resume();
            std::vector<int> vec{1, 2, 3};
            for (auto& i : vec)
            {
                SPDLOG_INFO(i);
                _sink = _sink.resume();
            }
            return std::move(_sink);
        });

    std::vector<std::string> vec{"a", "b", "c"};
    for (auto& i : vec)
    {
        SPDLOG_INFO(i);
        source = source.resume();
    }
}

void sub_to_sub()
{
    std::unique_ptr<continuation_t> sub_num;
    std::unique_ptr<continuation_t> sub_char;
    std::unique_ptr<continuation_t> sub_operator;

    SPDLOG_INFO("before creating sub_num");
    sub_num = std::make_unique<continuation_t>(context::callcc(
        [&sub_operator](continuation_t&& _sink)
        {
            spdlog::mdc::put("id", "sub_num");
            SPDLOG_INFO("creating sub_num");
            auto& main = _sink;
            main = main.resume(); // 跳主協程
            std::vector<int> vec{1, 2, 3, 4};
            for (auto& i : vec)
            {
                SPDLOG_INFO(i);
                *sub_operator = sub_operator->resume(); // 跳sub_operator，和sub_operator建立了一個通道
                auto& sub_char = _sink; // _sink就是sub_char
                sub_char = sub_char.resume(); // 跳sub_char
            }
            return std::move(_sink);
        }));
    SPDLOG_INFO("after creating sub_num");

    SPDLOG_INFO("before creating sub_char");
    sub_char = std::make_unique<continuation_t>(context::callcc(
        [&sub_num](continuation_t&& _sink)
        {
            spdlog::mdc::put("id", "sub_char");
            SPDLOG_INFO("creating sub_char");
            auto& main = _sink;
            main = main.resume(); // 跳主協程
            std::vector<std::string> vec{"a", "b", "c", "d"};
            for (auto& i : vec)
            {
                SPDLOG_INFO(i);
                *sub_num = sub_num->resume(); // 跳sub_num，此時可以認為和sub_num建立了一個通道
            }
            return std::move(_sink);
        }));
    SPDLOG_INFO("after creating sub_char");

    SPDLOG_INFO("before creating sub_operator");
    sub_operator = std::make_unique<continuation_t>(context::callcc(
        [](continuation_t&& _sink)
        {
            spdlog::mdc::put("id", "sub_operator");
            SPDLOG_INFO("creating sub_operator");
            auto& main = _sink;
            main = main.resume(); // 跳主協程
            std::vector<std::string> vec{"+", "-", "*", "/"};
            for (auto& i : vec)
            {
                SPDLOG_INFO(i);
                auto& sub_num = _sink;
                sub_num = sub_num.resume();
            }
            return std::move(_sink);
        }));
    SPDLOG_INFO("after creating sub_operator");

    *sub_char = sub_char->resume();
}

void multiple_continuation()
{
    std::unordered_map<int, std::unique_ptr<continuation_t>> continuation_cont;
    std::queue<int> awake_cont;

    for (auto i = 0u; i < 3; i++)
    {
        auto continuation = std::make_unique<continuation_t>(context::callcc(
            [i, &awake_cont](continuation_t&& _continuation)
            {
                auto id = i;
                std::vector<int> vec{1, 2, 3};
                for (auto& j : vec)
                {
                    SPDLOG_INFO("id: {} {}", id, j);
                    awake_cont.push(id);
                    _continuation = _continuation.resume();
                }
                return std::move(_continuation);
            }));
        continuation_cont[i] = std::move(continuation);
    }

    while (!awake_cont.empty())
    {
        auto i = awake_cont.front();
        SPDLOG_INFO("awake id: {}", i);
        auto iter = continuation_cont.find(i);
        if (iter != continuation_cont.end())
        {
            auto& continuation = *(iter->second);

            continuation = continuation.resume();
            if (!continuation)
            {
                continuation_cont.erase(iter);
                SPDLOG_INFO("child size: {}", continuation_cont.size());
            }
        }
        awake_cont.pop();
    }
}

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-15!!:%4#] %v");

    // single_continuation();

    sub_to_sub();

    // multiple_continuation();

    return 0;
}
