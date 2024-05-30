#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>
#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>

#include "time_count.h"

namespace asio = boost::asio;

namespace type
{
    using id = int;
}

namespace t = type;

struct Data
{
    std::vector<std::string> value;
    std::size_t offset = 0;
    t::id id;
};

void handle(asio::io_context& _io_context, std::shared_ptr<Data> _data, std::vector<std::string>& _output)
{
    auto& data = *_data;
    auto& offset = data.offset;
    auto& value = data.value;
    auto& id = data.id;

    if (offset < value.size())
    {
        SPDLOG_INFO("id: {} value: {}", id, value[offset]);
        _output[id] += value[offset];
        offset += 1;
        asio::post(_io_context, [&, _data] { handle(_io_context, _data, _output); });
    }
}

void accept_handle(asio::io_context& _io_context, int _count, int _id, std::vector<std::string>& _output)
{
    if (_id < _count)
    {
        auto data = std::make_shared<Data>();
        data->value = {"a", "b", "c"};
        data->id = _id;

        asio::post(_io_context, [&, data] { handle(_io_context, data, _output); });

        asio::post(_io_context, [&, _count, _id] { accept_handle(_io_context, _count, _id + 1, _output); });
    }
}

TEST_CASE("asio_callback", "[callback]")
{
    auto count = 3u;
    std::vector<std::string> output(count);

    asio::io_context io_context;
    asio::post(io_context, [&] { accept_handle(io_context, count, 0, output); });
    io_context.run();

    for (auto& i : output)
    {
        REQUIRE(i == "abc");
    }
}

int main(int argc, char* argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(argc, argv);
    return result;
}
