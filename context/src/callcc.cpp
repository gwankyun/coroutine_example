#include <map>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <queue>
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
    multiple_continuation();

    return 0;
}
