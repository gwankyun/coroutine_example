#include <map>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#define BOOST_ALL_NO_LIB 1
#include <boost/context/continuation.hpp>

namespace context = boost::context;
using continuation_t = context::continuation;

void example()
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

void example2()
{
    std::map<int, std::unique_ptr<continuation_t>> children;

    for (auto i = 0u; i < 3; i++)
    {
        children[i] = std::make_unique<continuation_t>(context::callcc(
            [i, &children](continuation_t&& _sink)
            {
                _sink = _sink.resume();

                // SPDLOG_INFO("id: {} read", i); // 異步讀
                // if (i < 2)
                // {
                //     *(children[i + 1]) = (children[i + 1])->resume();
                // }
                // else if (i == 2)
                // {
                //     *(children[0]) = (children[0])->resume();
                // }
                // SPDLOG_INFO("id: {} write", i); // 異步寫

                std::vector<int> vec{1, 2, 3};
                for (auto& v : vec)
                {
                    SPDLOG_INFO("id: {} {}", i, v);
                    // _sink = _sink.resume();
                    if (i < 2)
                    {
                        if (*(children[i + 1]))
                        {
                            *(children[i + 1]) = (children[i + 1])->resume();
                        }
                    }
                    else
                    {
                        // *(children[0]) = (children[0])->resume();
                        // _sink = _sink.resume();
                        if (*(children[0]))
                        {
                            *(children[0]) = (children[0])->resume();
                        }
                    }
                }

                return std::move(_sink);
            }));
    }

    *(children[0]) = (children[0])->resume();
}

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-50!!:%4#] %v");

    // example1();

    // example3();
    // example4();

    // example();
    example2();

    return 0;
}
