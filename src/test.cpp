#include <spdlog/spdlog.h>

#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include <boost/asio.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/scope/defer.hpp>

import std;

using namespace std::chrono_literals;

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

TEST_CASE("async", "[callback]")
{
    boost::asio::io_context context;

    using boost::asio::steady_timer;
    using boost::system::error_code;

    std::string result = "0";

    auto time = 100ms;

    SPDLOG_INFO("");

    {
        auto accept = std::make_shared<steady_timer>(context, time);

        accept->async_wait(
            [&, accept](error_code _ec)
            {
                if (_ec)
                {
                    SPDLOG_ERROR("{}", _ec.message());
                    return;
                }
                SPDLOG_INFO("accept");
                result += "1";
                auto read = std::make_shared<steady_timer>(context, time);
                read->async_wait(
                    [&, read](error_code _ec)
                    {
                        if (_ec)
                        {
                            SPDLOG_ERROR("{}", _ec.message());
                            return;
                        }
                        SPDLOG_INFO("read");
                        result += "2";
                        auto write = std::make_shared<steady_timer>(context, time);
                        write->async_wait(
                            [&, write](error_code _ec)
                            {
                                if (_ec)
                                {
                                    SPDLOG_ERROR("{}", _ec.message());
                                    return;
                                }
                                SPDLOG_INFO("write");
                                result += "3";
                            });
                    });
            });
    }

    SPDLOG_INFO("");

    context.run();
    REQUIRE(result == "0123");
}

using boost::asio::steady_timer;
using boost::system::error_code;

struct SwitchData
{
    int state;
    std::string result;
    int i;
};

void on_switch(SwitchData& _data, boost::asio::io_context& context)
{
    auto time = 100ms;

    auto cb = [&](std::shared_ptr<steady_timer> _timer, std::string _message)
    {
        return [&, _timer, _message](error_code _ec)
        {
            if (_ec)
            {
                SPDLOG_ERROR("{}", _ec.message());
                return;
            }
            _data.result += _message;
            on_switch(_data, context);
        };
    };

    CORO_BEGIN(_data.state);

    [&]
    {
        auto accept = std::make_shared<steady_timer>(context, time);
        accept->async_wait(cb(accept, "1"));
    }();
    CORO_YIELD(_data.state);

    for (_data.i = 0; _data.i < 3; ++_data.i)
    {
        SPDLOG_INFO("{}", _data.i);
        [&]
        {
            auto read = std::make_shared<steady_timer>(context, time);
            read->async_wait(cb(read, "2"));
        }();
        CORO_YIELD(_data.state);
    }

    [&]
    {
        auto write = std::make_shared<steady_timer>(context, time);
        write->async_wait(cb(write, "3"));
    }();
    CORO_YIELD(_data.state);

    CORO_END();
}

TEST_CASE("async", "[switch]")
{
    boost::asio::io_context context;

    std::string result = "0";
    int state = 0;

    SwitchData data{state, result, 0};

    on_switch(data, context);

    context.run();
    REQUIRE(data.result == "012223");
}

TEST_CASE("async", "[coroutine2]")
{
    boost::asio::io_context context;
    namespace coro2 = boost::coroutines2;

    std::string result = "0";

    auto time = 100ms;

    std::queue<std::tuple<int, error_code>> awake_cont;
    using coro_type = coro2::coroutine<error_code>;
    std::unordered_map<int, std::unique_ptr<coro_type::push_type>> coro_cont;

    auto make_coro = [&](int _id)
    {
        return [&, _id](typename coro_type::pull_type& _push)
        {
            auto awake = [&] { awake_cont.push(std::make_tuple(_id, error_code())); };
            BOOST_SCOPE_DEFER[&]
            {
                awake();
            };

            auto cb = [&](std::string _message)
            {
                return [&, _message](error_code)
                {
                    result += _message;
                    awake();
                };
            };

            error_code e;

            SPDLOG_INFO("");
            steady_timer accept(context, time);
            accept.async_wait(cb("1"));
            _push();
            e = _push.get();
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return;
            }

            SPDLOG_INFO("");
            for (int i = 0; i < 3; ++i)
            {
                steady_timer read(context, time);
                read.async_wait(cb("2"));
                _push();
                e = _push.get();
                if (e)
                {
                    SPDLOG_ERROR("{}", e.message());
                    return;
                }
            }

            SPDLOG_INFO("");
            steady_timer write(context, time);
            write.async_wait(cb("3"));
            _push();
            e = _push.get();
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return;
            }
        };
    };

    {
        auto id = 0;
        coro_cont[id] = std::make_unique<coro_type::push_type>(make_coro(id));
        (*coro_cont[id])(error_code());
    }

    while (!coro_cont.empty())
    {
        context.run();
        if (!awake_cont.empty())
        {
            auto [id, err] = awake_cont.front();
            awake_cont.pop();

            auto iter = coro_cont.find(id);
            if (iter != coro_cont.end())
            {
                auto& pull = *iter->second;
                if (!pull)
                {
                    coro_cont.erase(id);
                    continue;
                }
                pull(err);
            }
        }
    }
    REQUIRE(result == "012223");
}

int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
