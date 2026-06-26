module;

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

module test.callback;

import std;

using namespace std::chrono_literals;
using boost::asio::steady_timer;
using boost::system::error_code;

auto g_time = 100ms;

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

void on_connection(error_code ec, std::shared_ptr<ConnectionData> data)
{
    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        return;
    }
    auto connection = data->connection;

    if (data->i == 0)
    {
        (*data->result)[data->id] = std::to_string(data->id);
        data->i++;
        connection->expires_after(g_time);
        connection->async_wait([=](error_code _ec) { on_connection(_ec, data); });
    }
    else if (data->i <= 3)
    {
        (*data->result)[data->id] += "r";
        data->i++;
        connection->expires_after(g_time);
        connection->async_wait([=](error_code _ec) { on_connection(_ec, data); });
    }
    else
    {
        (*data->result)[data->id] += "w";
    }
    //if (data->i <= 3)
    //{
    //    (*data->result)[data->id] += "r";
    //    data->i++;
    //    connection->expires_after(g_time);
    //    connection->async_wait([=](error_code _ec) { on_connection(_ec, data); });
    //}
    //else
    //{
    //    (*data->result)[data->id] += "w";

    //    // connection->expires_after(g_time);
    //    // connection->async_wait([connection, data](error_code _ec) {
    //    //     if (_ec)
    //    //     {
    //    //         SPDLOG_ERROR("{}", _ec.message());
    //    //         return;
    //    //     }
    //    //     (*data->result)[data->id] += "w";
    //    // });
    //}

    //(*data->result)[data->id] += "r";
    // data->i++;
    // if (data->i < 3)
    //{
    //    connection->expires_after(g_time);
    //    connection->async_wait([=](error_code _ec) { on_connection(_ec, data); });
    //}
    // else
    //{
    //    connection->expires_after(g_time);
    //    connection->async_wait([connection, data](error_code _ec) {
    //        if (_ec)
    //        {
    //            SPDLOG_ERROR("{}", _ec.message());
    //            return;
    //        }
    //        (*data->result)[data->id] += "w";
    //    });
    //}
}

void on_accept(error_code ec, std::shared_ptr<AcceptorData> data)
{
    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        return;
    }
    SPDLOG_INFO("");
    auto acceptor = data->acceptor;

    if (data->id < 3)
    {
        data->id++;

        auto conn_timer = std::make_shared<steady_timer>(acceptor->get_executor());
        auto conn_data = std::make_shared<ConnectionData>();
        conn_data->result = data->result;
        conn_data->id = data->id;
        conn_data->connection = conn_timer;
        conn_timer->expires_after(g_time);
        conn_timer->async_wait([=](error_code _ec) { on_connection(_ec, conn_data); });

        acceptor->expires_after(g_time);
        acceptor->async_wait([=](error_code _ec) { on_accept(_ec, data); });
    }
}

std::unordered_map<int, std::string> test_callback()
{
    SPDLOG_INFO("");
    boost::asio::io_context context;
    std::unordered_map<int, std::string> result;
    {
        auto acceptor = std::make_shared<steady_timer>(context);
        auto data = std::make_shared<AcceptorData>();
        data->acceptor = acceptor;
        data->result = &result;
        acceptor->expires_after(g_time);
        acceptor->async_wait([=](error_code ec) { on_accept(ec, data); });
    }
    SPDLOG_INFO("");
    context.run();
    return result;
}
