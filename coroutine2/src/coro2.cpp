﻿// module;
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#define BOOST_ALL_NO_LIB 1
#include <boost/coroutine2/all.hpp>
#include <functional>
// module coro2;
// import std.core;

namespace coro2 = boost::coroutines2;

void func(coro2::coroutine<void>::push_type& _yield)
{
    std::vector<int> vec{1, 2, 3};
    SPDLOG_INFO("{}", vec[0]);
    _yield();
    SPDLOG_INFO("{}", vec[1]);
    _yield();
    SPDLOG_INFO("{}", vec[2]);
}

void func2(coro2::coroutine<std::string>::push_type& _yield)
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

template <typename T = void>
using pull_type = typename coro2::coroutine<T>::pull_type;

template <typename T = void>
using push_type = typename coro2::coroutine<T>::push_type;

void single_coroutine()
{
    using pull_t = pull_type<>;
    using push_t = push_type<>;

    pull_t pull(
        [](push_t& _push)
        {
            SPDLOG_INFO("#1");
            _push();
            std::vector<int> vec{1, 2, 3};
            for (auto& i : vec)
            {
                SPDLOG_INFO(i);
                _push();
            }
        });

    SPDLOG_INFO("#2");

    std::vector<std::string> vec{"a", "b", "c"};
    for (auto& i : vec)
    {
        SPDLOG_INFO(i);
        pull();
    }
    SPDLOG_INFO("pull: {}", bool(pull));
    if (pull)
    {
        pull(); // 跑最後一行
        SPDLOG_INFO("pull: {}", bool(pull));
    }
}

struct OnExit
{
    using fn_t = std::function<void()>;
    OnExit(fn_t _fn) : fn(_fn)
    {
    }
    ~OnExit()
    {
        fn();
    }
    fn_t fn;
};

void multiple_coroutine()
{
    using pull_t = pull_type<>;
    using push_t = push_type<>;

    std::unordered_map<int, std::unique_ptr<pull_t>> coro_cont;
    std::queue<int> awake_cont;

    for (auto i = 0u; i < 3; i++)
    {
        auto pull = std::make_unique<pull_t>(
            [i, &awake_cont](push_t& _push)
            {
                auto id = i;
                OnExit onExit([&awake_cont, &id] { awake_cont.push(id); });
                std::vector<int> vec{1, 2, 3};
                for (auto& j : vec)
                {
                    SPDLOG_INFO("id: {} {}", id, j);
                    if (j == 2)
                    {
                        return;
                    }
                    awake_cont.push(id);
                    _push();
                }
            });
        coro_cont[i] = std::move(pull);
    }

    while (!awake_cont.empty())
    {
        auto i = awake_cont.front();
        SPDLOG_INFO("awake id: {}", i);
        auto iter = coro_cont.find(i);
        if (iter != coro_cont.end())
        {
            auto& resume = *(iter->second);
            if (!resume)
            {
                coro_cont.erase(iter);
                SPDLOG_INFO("child size: {}", coro_cont.size());
            }
            else
            {
                resume();
            }
        }
        awake_cont.pop();
    }
}

void example3()
{
    pull_type<> pull(
        [](push_type<>& _push)
        {
            _push();
            SPDLOG_INFO("1");
            _push();
            SPDLOG_INFO("2");
            _push();
            SPDLOG_INFO("3");

            // std::vector<std::unique_ptr<pull_type<>>> child(2);
            std::map<int, std::unique_ptr<pull_type<>>> child;

            for (auto i = 0u; i < 3; i++)
            {
                child[i] = std::make_unique<pull_type<>>(
                    [i](push_type<>& _push_child)
                    {
                        _push_child();
                        SPDLOG_INFO("id: {} read", i);  // 異步讀
                        _push_child();                  // 返回主協程
                        SPDLOG_INFO("id: {} write", i); // 異步寫
                    });
            }

            while (!child.empty())
            {
                for (auto it = child.begin(); it != child.end();)
                {
                    auto& c = *(it->second);
                    if (c) // 判斷幾時喚醒子協程
                    {
                        SPDLOG_INFO("");
                        c();
                        it++;
                    }
                    else
                    {
                        it = child.erase(it);
                    }
                }
            }

            // int id = 0;
            ////for (auto& i : child)
            // for (auto it = child.m)
            //{
            //     i = std::make_unique<pull_type<>>(([id](push_type<>& _push_child)
            //         {
            //             _push_child();
            //             SPDLOG_INFO("{} 1", id);
            //             _push_child();
            //             SPDLOG_INFO("{} 2", id);
            //         }));
            //     id++;
            // }

            // bool flag = true;
            // while (flag)
            //{
            //     for (auto& i : child)
            //     {
            //         flag = false;
            //         SPDLOG_INFO("");
            //         if (*i)
            //         {
            //             SPDLOG_INFO("");
            //             (*i)();
            //             flag = true;
            //         }
            //     }
            // }

            // pull_type<> pull_child([](push_type<>& _push_child)
            //     {
            //         _push_child();
            //         SPDLOG_INFO(" 1");
            //         _push_child();
            //         SPDLOG_INFO(" 2");
            //     });

            // while (pull_child)
            //{
            //     SPDLOG_INFO("");
            //     pull_child();
            // }
        });

    SPDLOG_INFO("a");
    pull();
    SPDLOG_INFO("b");
    pull();
    SPDLOG_INFO("c");
    pull();
}

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-15!!:%4#] %v");

    // example1();

    // example3();

    // single_coroutine();

    multiple_coroutine();

    return 0;
}
