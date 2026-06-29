module;

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

module test.callback;

import std;

using namespace std::chrono_literals;
using boost::asio::steady_timer;
using boost::system::error_code;

auto g_time = 100ms;

namespace Callback
{

    struct ConnectionData
    {
        std::unordered_map<int, std::string>* result = nullptr;
        int id = 0;
        int i = 0;
        std::unique_ptr<steady_timer> connection;
        using TableType = std::unordered_map<int, std::unique_ptr<ConnectionData>>;
        static TableType table;
    };

    ConnectionData::TableType ConnectionData::table;

    struct AcceptorData
    {
        std::unordered_map<int, std::string>* result = nullptr;
        int id = 0;
        std::unique_ptr<steady_timer> acceptor;
    };

    void on_connect(error_code ec, ConnectionData& data)
    {
        if (ec) // #1
        {
            SPDLOG_ERROR("{}", ec.message());
            return;
        }
        auto& connection = data.connection;
        auto& i = data.i;
        auto id = data.id;
        auto& result = *data.result;

        auto cb = [&](error_code _ec) { on_connect(_ec, data); };

        if (i == 0)
        {
            result[id] = std::to_string(id);
            i++;
            connection->expires_after(g_time);
            connection->async_wait(cb); // -> #1
        }
        else if (i <= 3)
        {
            result[id] += "r";
            i++;
            connection->expires_after(g_time);
            connection->async_wait(cb); // -> #1
        }
        else
        {
            result[id] += "w";
            ConnectionData::table.erase(id);
        }
    }

    void on_accept(error_code ec, AcceptorData& data)
    {
        if (ec) // #2
        {
            SPDLOG_ERROR("{}", ec.message());
            return;
        }
        auto& id = data.id;
        auto& acceptor = data.acceptor;
        auto result = data.result;

        auto cb = [&](error_code _ec) { on_accept(_ec, data); };

        if (id <= 3)
        {
            auto& table = ConnectionData::table;
            table[id] = std::make_unique<ConnectionData>();
            auto& conn = *table[id];
            conn.id = id;
            conn.result = result;
            conn.connection = std::make_unique<steady_timer>(acceptor->get_executor());
            auto& connection = conn.connection;
            connection->expires_after(g_time);
            connection->async_wait([&](error_code ec) { on_connect(ec, conn); }); // -> #1

            acceptor->expires_after(g_time);
            acceptor->async_wait(cb); // -> #2
            id++;
        }
    }

} // namespace Callback

std::unordered_map<int, std::string> test_callback()
{
    SPDLOG_INFO("");
    boost::asio::io_context context;
    std::unordered_map<int, std::string> result;

    Callback::AcceptorData data;
    data.acceptor = std::make_unique<steady_timer>(context);
    data.result = &result;

    data.acceptor->expires_after(g_time);
    data.acceptor->async_wait([&](error_code _ec) {
        data.id++;
        Callback::on_accept(_ec, data);
    }); // -> #2

    SPDLOG_INFO("");
    context.run();
    return result;
}
