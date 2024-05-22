#include <memory>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include "time.hpp"

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

void handle(asio::io_context& _io_context, std::shared_ptr<Data> _data)
{
    auto& data = *_data;
    auto& offset = data.offset;
    auto& value = data.value;
    auto& id = data.id;

    if (offset < value.size())
    {
        SPDLOG_INFO("id: {} value: {}", id, value[offset]);
        offset += 1;
        asio::post(_io_context, [&, _data] { handle(_io_context, _data); });
    }
}

void accept_handle(asio::io_context& _io_context, int _count, int _id)
{
    if (_id < _count)
    {
        auto data = std::make_shared<Data>();
        data->value.push_back("a");
        data->value.push_back("b");
        data->value.push_back("c");
        data->id = _id;

        asio::post(_io_context, [&, data] { handle(_io_context, data); });

        asio::post(_io_context, [&, _count, _id] { accept_handle(_io_context, _count, _id + 1); });
    }
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    auto count = common::time_count(
        [&]
        {
            asio::post(io_context, [&] { accept_handle(io_context, 3, 0); });
            io_context.run();
        });

    SPDLOG_INFO("used times: {}", count);

    return 0;
}
