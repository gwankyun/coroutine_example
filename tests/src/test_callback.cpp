module;

#include <boost/asio.hpp>
#include <spdlog.hpp>

module test.callback;

import std;
import spdlog;

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
        std::shared_ptr<steady_timer> connection;
    };

    struct AcceptorData
    {
        std::unordered_map<int, std::string>* result = nullptr;
        int id = 0;
        std::shared_ptr<steady_timer> acceptor;
    };

    void on_connect(error_code ec, std::shared_ptr<ConnectionData> _data)
    {
        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            return;
        }
        auto& data = *_data;
        auto& connection = *data.connection;
        auto& i = data.i;
        auto id = data.id;
        auto& result = *data.result;

        auto cb = [=](error_code _ec) { on_connect(_ec, _data); };

        if (i == 0)
        {
            result[id] = std::to_string(id);
            i++;
            connection.expires_after(g_time);
            connection.async_wait(cb);
        }
        else if (i <= 3)
        {
            result[id] += "r";
            i++;
            connection.expires_after(g_time);
            connection.async_wait(cb);
        }
        else
        {
            result[id] += "w";
        }
    }

    void on_accept(error_code ec, std::shared_ptr<AcceptorData> _data)
    {
        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            return;
        }
        auto& data = *_data;
        auto& id = data.id;
        auto& acceptor = *data.acceptor;
        auto result = data.result;

        auto cb = [=](error_code _ec) { on_accept(_ec, _data); };

        if (id <= 3)
        {
            auto conn = std::make_shared<ConnectionData>();
            conn->id = id;
            conn->result = result;
            conn->connection = std::make_shared<steady_timer>(acceptor.get_executor());
            conn->connection->expires_after(g_time);
            conn->connection->async_wait([=](error_code ec) { on_connect(ec, conn); });

            acceptor.expires_after(g_time);
            acceptor.async_wait(cb);
            id++;
        }
    }

} // namespace Callback

std::unordered_map<int, std::string> test_callback()
{
    SPDLOG_INFO("");
    boost::asio::io_context context;
    std::unordered_map<int, std::string> result;

    auto data = std::make_shared<Callback::AcceptorData>();
    data->acceptor = std::make_shared<steady_timer>(context);
    data->result = &result;
    auto& acceptor = *data->acceptor;

    acceptor.expires_after(g_time);
    acceptor.async_wait([=](error_code _ec) {
        data->id++;
        Callback::on_accept(_ec, data);
    });

    SPDLOG_INFO("");
    context.run();
    return result;
}
