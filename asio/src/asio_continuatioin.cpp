#include <boost/asio.hpp>
#include <boost/system.hpp> // boost::system::error_code
#include <spdlog/spdlog.h>
#include <string>
#define BOOST_ALL_NO_LIB 1
#include <boost/context/continuation.hpp>
#include <queue>
#include <unordered_map>

namespace context = boost::context;
using continuation_t = context::continuation;

namespace asio = boost::asio;
using error_code_t = boost::system::error_code;

void handle_old(asio::io_context& _io_context)
{
    std::unique_ptr<continuation_t> main;
    std::unique_ptr<continuation_t> sub;

    bool flag = false;

    sub = std::make_unique<continuation_t>(context::callcc(
        [&_io_context, &flag, &main](continuation_t&& _sink)
        {
            _sink = _sink.resume();
            error_code_t error;
            auto cb = [&flag, &error](error_code_t _e = error_code_t{})
            {
                error = _e;
                flag = true;
            };

            asio::post(_io_context, cb);
            flag = false;
            *main = main->resume();

            SPDLOG_INFO("read");
            asio::post(_io_context, cb);
            flag = false;
            *main = main->resume();

            SPDLOG_INFO("write");
            asio::steady_timer timer(_io_context, asio::chrono::seconds(1));
            timer.async_wait(cb);
            flag = false;
            *main = main->resume();

            if (error)
            {
                SPDLOG_INFO("{}", error.message());
                return std::move(_sink);
            }
            SPDLOG_INFO("timeout");
            return std::move(_sink);
        }));

    main = std::make_unique<continuation_t>(context::callcc(
        [&_io_context, &flag](continuation_t&& _sink)
        {
            _sink = _sink.resume();
            auto& sub = _sink;
            while (sub)
            {
                _io_context.run();
                if (flag)
                {
                    sub = sub.resume();
                }
            }
            return std::move(_sink);
        }));

    *sub = sub->resume();
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

void handle(asio::io_context& _io_context)
{
    std::unique_ptr<continuation_t> main;

    // 協程容器
    std::unordered_map<int, std::unique_ptr<continuation_t>> continuation_cont;

    // 待喚醒協程隊列。
    std::queue<int> awake_cont;

    auto create_continuation = [&_io_context, &awake_cont, &main](int id)
    {
        return [&_io_context, &awake_cont, id, &main](continuation_t&& _sink)
        {
            OnExit onExit([&awake_cont, id] { awake_cont.push(id); });
            std::vector<int> data{1, 2, 3};
            for (auto& i : data)
            {
                SPDLOG_INFO("id: {} value: {}", id, i);
                asio::post(_io_context, [&awake_cont, id] { awake_cont.push(id); });
                _sink = _sink.resume();
            }
            return std::move(_sink);
        };
    };

    for (auto id = 0; id < 3; id++)
    {
        continuation_cont[id] = std::make_unique<continuation_t>(context::callcc(create_continuation(id)));
    }

    main = std::make_unique<continuation_t>(context::callcc(
        [&_io_context, &continuation_cont, &awake_cont](continuation_t&& _sink)
        {
            while (!continuation_cont.empty())
            {
                // 實際執行異步操作
                _io_context.run();

                if (!awake_cont.empty())
                {
                    SPDLOG_INFO("awake_cont size: {}", awake_cont.size());
                }

                // 簡單的協程調度，按awake_cont中先進先出的順序喚醒協程。
                while (!awake_cont.empty())
                {
                    auto i = awake_cont.front();
                    awake_cont.pop();
                    SPDLOG_INFO("awake id: {}", i);
                    auto iter = continuation_cont.find(i);
                    if (iter != continuation_cont.end())
                    {
                        auto& continuation = *(iter->second);
                        if (!continuation)
                        {
                            continuation_cont.erase(iter);
                            SPDLOG_INFO("child size: {}", continuation_cont.size());
                            continue;
                        }
                        continuation = continuation.resume();
                    }
                }
            }

            return std::move(_sink);
        }));
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    // handle_old(io_context);
    handle(io_context);

    return 0;
}
