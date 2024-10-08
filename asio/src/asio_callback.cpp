﻿#include <memory>
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
    using output = std::unordered_map<id, std::string>;
} // namespace type

namespace t = type;

struct Data
{
    std::vector<std::string> value;
    t::id id;
};

void handle(asio::io_context& _io_context, std::shared_ptr<Data> _data, t::output& _output)
{
    auto& data = *_data;
    auto& value = data.value;
    auto& id = data.id;

    if (!value.empty())
    {
        auto v = value.back();
        _output[id] += v;
        value.pop_back();
        SPDLOG_INFO("id: {} value: {}", id, v);
        auto handle_next = [&, _data] { handle(_io_context, _data, _output); };
        asio::post(_io_context, handle_next);
    }
}

void accept_handle(asio::io_context& _io_context, int _count, int _id, t::output& _output)
{
    if (_id < _count)
    {
        auto data = std::make_shared<Data>();
        data->value = {"a", "b", "c"};
        data->id = _id;

        auto handle_data = [&, data] { handle(_io_context, data, _output); };
        asio::post(_io_context, handle_data);

        auto accept_next = [&, _count, _id] { accept_handle(_io_context, _count, _id + 1, _output); };
        asio::post(_io_context, accept_next);
    }
}

TEST_CASE("asio_callback", "[callback]")
{
    auto count = 3u;
    t::output output(count);

    asio::io_context io_context;
    asio::post(io_context, [&] { accept_handle(io_context, count, 0, output); });
    io_context.run();

    for (auto& i : output)
    {
        REQUIRE(i.second == "cba");
    }
}

int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
