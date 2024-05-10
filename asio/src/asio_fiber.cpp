#include <boost/asio.hpp>
#include <boost/system.hpp> // boost::system::error_code
#include <spdlog/spdlog.h>
#include <string>
#define BOOST_ALL_NO_LIB 1
#include <boost/context/fiber.hpp>
#include <queue>
#include <unordered_map>
#include <chrono>

namespace context = boost::context;
using fiber_t = context::fiber;

namespace asio = boost::asio;
using error_code_t = boost::system::error_code;

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

void handle(asio::io_context& _io_context, bool _manage_on_sub, int _count)
{
    std::unique_ptr<fiber_t> main;

    // 協程容器
    std::unordered_map<int, std::unique_ptr<fiber_t>> fiber_cont;

    // 待喚醒協程隊列。
    std::queue<int> awake_cont;

    auto create_fiber = [&_io_context, &awake_cont, &main](int id)
    {
        return [&_io_context, &awake_cont, id, &main](fiber_t&& _sink)
        {
            auto cb = [&awake_cont, id] { awake_cont.push(id); };
            OnExit onExit(cb);
            std::vector<int> data{1, 2, 3};
            for (auto& i : data)
            {
                SPDLOG_INFO("id: {} value: {}", id, i);
                asio::post(_io_context, cb);
                _sink = std::move(_sink).resume();
            }
            return std::move(_sink);
        };
    };

    for (auto id = 0; id < _count; id++)
    {
        fiber_cont[id] = std::make_unique<fiber_t>(create_fiber(id));
        // 和continuation不一樣，創建後不會自動執行。
        auto& fiber = *fiber_cont[id];
        fiber = std::move(fiber).resume();
    }

    auto main_fiber = [&]
    {
        while (!fiber_cont.empty())
        {
            // 實際執行異步操作
            _io_context.run();

            if (!awake_cont.empty())
            {
                SPDLOG_DEBUG("awake_cont size: {}", awake_cont.size());
            }

            // 簡單的協程調度，按awake_cont中先進先出的順序喚醒協程。
            while (!awake_cont.empty())
            {
                auto i = awake_cont.front();
                awake_cont.pop();
                SPDLOG_DEBUG("awake id: {}", i);
                auto iter = fiber_cont.find(i);
                if (iter != fiber_cont.end())
                {
                    auto& fiber = *(iter->second);
                    if (!fiber)
                    {
                        fiber_cont.erase(iter);
                        SPDLOG_DEBUG("child size: {}", fiber_cont.size());
                        continue;
                    }
                    fiber = std::move(fiber).resume();
                }
            }
        }
    };

    if (_manage_on_sub)
    {
        SPDLOG_INFO("create main");
        main = std::make_unique<fiber_t>(
            [&main_fiber](fiber_t&& _sink)
            {
                SPDLOG_INFO("enter main");
                main_fiber();
                return std::move(_sink);
            });

        SPDLOG_INFO("resume main");
        *main = std::move(*main).resume();
    }
    else
    {
        main_fiber();
    }
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;
    namespace chrono = std::chrono;

    auto start = chrono::steady_clock::now();
    handle(io_context, true, 3);
    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    SPDLOG_INFO("used times: {}", duration.count());

    return 0;
}
