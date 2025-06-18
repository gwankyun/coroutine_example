module;
#include "use_module.h"

#if !USE_STD_MODULE
#  include <memory>
#  include <string>
#  include <unordered_map>
#  include <vector>
#endif

#include "catch2.h"

#include <spdlog/spdlog.h>

#if !USE_BOOST_ASIO_MODULE
#  include <boost/asio.hpp>
#endif

// #include "time_count.h"

export module asio_callback;

#if USE_STD_MODULE
import std;
#endif

#if USE_CATCH2_MODULE
import catch2.compat;
#endif

#if USE_SPDLOG_MODULE
import spdlog;
#endif

#if USE_BOOST_ASIO_MODULE
import boost.asio;
namespace asio = boost_asio;
#endif

namespace type
{
    using id = int;
    using output = std::unordered_map<id, std::string>;
} // namespace type

namespace t = type;

struct Data
{
    t::id id;
    std::string value;
    std::size_t offset = 0;
};

void handle(asio::io_context& _io_context, std::shared_ptr<Data> _data, t::output& _output)
{
    auto& data = *_data;
    auto& value = data.value;
    auto& id = data.id;
    auto& offset = data.offset;

    if (offset < value.size())
    {
        auto v = value[offset];
        _output[id] += v;
        SPDLOG_INFO("id: {} value: {}", id, v);
        offset++;
        auto handle_next = [&, _data] { handle(_io_context, _data, _output); };
        asio::post(_io_context, handle_next);
    }
}

void accept_handle(asio::io_context& _io_context, int _count, int _id, t::output& _output)
{
    if (_id < _count)
    {
        auto data = std::make_shared<Data>();
        data->value = "abc";
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
        REQUIRE(i.second == "abc");
    }
}

export int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
