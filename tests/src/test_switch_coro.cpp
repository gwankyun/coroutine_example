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

struct ConnectionData
{
    int state = 0;
    std::unordered_map<int, std::string>* result = nullptr;
    int i = 0;
    std::unique_ptr<steady_timer> connection; // = nullptr;
};

struct AcceptorData
{
    int state = 0;
    std::unordered_map<int, std::string>* result = nullptr;
    int i = 0;
    std::unique_ptr<steady_timer> acceptor; // = nullptr;
};

std::unordered_map<int, std::unique_ptr<ConnectionData>> connection_map;

void on_connect(error_code ec, ConnectionData* _data)
{
    //SPDLOG_INFO("on_connect: state={}, i={}", _data.state, _data.i);
    auto time = 100ms;
    auto& connection = _data->connection;
    auto result = _data->result;

    auto cb = [_data]() { return [_data](error_code _ec) { on_connect(_ec, _data); }; };

    CORO_BEGIN(_data->state);

    connection->expires_after(time);
    connection->async_wait(cb());
    CORO_YIELD(_data->state);

    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        return;
    }
    //*result += "1";
    (*result)[_data->i] += std::to_string(_data->i);

    // SPDLOG_INFO("");

    for (_data->i = 0; _data->i < 3; ++_data->i)
    {
        SPDLOG_INFO("{}", _data->i);
        connection->expires_after(time);
        connection->async_wait(cb());
        CORO_YIELD(_data->state);
        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            return;
        }
        (*result)[_data->i] += "r";
    }

    connection->expires_after(time);
    connection->async_wait(cb());
    CORO_YIELD(_data->state);

    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        return;
    }
    (*result)[_data->i] += "w";

    CORO_END();
}

void on_accept(error_code ec, AcceptorData* _data)
{
    // SPDLOG_INFO("on_connect: state={}, i={}", _data->state, _data->i);
    auto time = 100ms;
    auto& acceptor = _data->acceptor;
    auto result = _data->result;
     //auto state = _data->state;

    auto cb = [_data]() { return [_data](error_code _ec) { on_accept(_ec, _data); }; };

    CORO_BEGIN(_data->state);

    for (_data->i = 1; _data->i < 3; ++_data->i)
    {
        SPDLOG_INFO("{}", _data->i);
        acceptor->expires_after(time);
        acceptor->async_wait(cb());
        CORO_YIELD(_data->state);
        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            return;
        }

        auto conn = std::make_unique<ConnectionData>();
        conn->connection = std::make_unique<steady_timer>(_data->acceptor->get_executor());
        conn->i = _data->i;
        conn->result = _data->result;
        connection_map[_data->i] = std::move(conn);
        on_connect(error_code{}, connection_map[_data->i].get());

        SPDLOG_INFO("connection: {}", _data->i);
        SPDLOG_INFO("connection size: {}", connection_map.size());

    }

    CORO_END();
}

std::unordered_map<int, std::string> test_switch_coro()
{
    SPDLOG_INFO("");
    boost::asio::io_context context;

    std::unordered_map<int, std::string> result; // = "0";

    AcceptorData data;
    data.result = &result;
    data.acceptor = std::make_unique<steady_timer>(context);

    SPDLOG_INFO("");
    on_accept(error_code{}, &data);

    context.run();

    for (auto& i : result)
    {
        SPDLOG_INFO("result[{}]: {}", i.first, i.second);
    }

    return result;
}
