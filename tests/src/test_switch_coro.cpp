module;

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

module test.switch_coro;

import std;

using namespace std::chrono_literals;
using boost::asio::steady_timer;
using boost::system::error_code;

#define CORO_BEGIN(_state) \
    switch (_state) \
    { \
    case 0:

#define CORO_YIELD(_state) \
    do \
    { \
        _state = __LINE__; \
        return; \
    case __LINE__:; \
    } while (false)

#define CORO_END() }

namespace SwitchCoro
{

    struct ConnectionData
    {
        int state = 0;
        std::unordered_map<int, std::string>* result = nullptr;
        std::unordered_map<int, std::unique_ptr<ConnectionData>>* table = nullptr;
        int id = 0;
        int i = 0;
        std::unique_ptr<steady_timer> connection;
    };

    using ConnectionTable = std::unordered_map<int, std::unique_ptr<ConnectionData>>;

    struct AcceptorData
    {
        int state = 0;
        std::unordered_map<int, std::string>* result = nullptr;
        int i = 0;
        std::unique_ptr<steady_timer> acceptor;
        ConnectionTable table;
    };

    void on_connect(error_code _ec, ConnectionData* _data)
    {
        auto time = 100ms;
        auto& state = _data->state;
        auto& connection = _data->connection;
        auto& i = _data->i;
        auto id = _data->id;
        auto& result = *(_data->result);
        auto table = _data->table;

        auto cb = [=](error_code ec) { on_connect(ec, _data); };

        CORO_BEGIN(state);

        connection->expires_after(time);
        connection->async_wait(cb);
        CORO_YIELD(state);
        if (_ec)
        {
            SPDLOG_ERROR("{}", _ec.message());
            return;
        }

        result[id] = std::to_string(id);

        for (i = 0; i < 3; ++i)
        {
            connection->expires_after(time);
            connection->async_wait(cb);
            CORO_YIELD(state);
            if (_ec)
            {
                SPDLOG_ERROR("{}", _ec.message());
                return;
            }

            result[id] += "r";
        }

        connection->expires_after(time);
        connection->async_wait(cb);
        CORO_YIELD(state);
        if (_ec)
        {
            SPDLOG_ERROR("{}", _ec.message());
            return;
        }

        result[id] += "w";

        CORO_END();

        table->erase(id);
    }

    void on_accept(error_code _ec, AcceptorData& _data)
    {
        auto time = 100ms;
        auto& state = _data.state;
        auto& acceptor = _data.acceptor;
        auto& i = _data.i;
        auto& table = _data.table;

        auto cb = [&_data](error_code ec) { on_accept(ec, _data); };

        CORO_BEGIN(state);

        for (i = 1; i <= 3; ++i)
        {
            acceptor->expires_after(time);
            acceptor->async_wait(cb);
            CORO_YIELD(state);
            if (_ec)
            {
                SPDLOG_ERROR("{}", _ec.message());
                return;
            }

            auto conn = std::make_unique<ConnectionData>();
            conn->result = _data.result;
            conn->connection = std::make_unique<steady_timer>(acceptor->get_executor());
            conn->id = i;
            conn->table = &table;
            table[i] = std::move(conn);
            on_connect(error_code{}, table[i].get());
        }

        CORO_END();
    }

} // namespace SwitchCoro

std::unordered_map<int, std::string> test_switch_coro()
{
    boost::asio::io_context context;

    std::unordered_map<int, std::string> result;

    SwitchCoro::AcceptorData data;
    data.result = &result;
    data.acceptor = std::make_unique<steady_timer>(context);

    data.acceptor->expires_after(100ms);
    data.acceptor->async_wait([&](error_code _ec) {
        data.i = 1;
        SwitchCoro::on_accept(_ec, data);
    });

    context.run();
    return result;
}
