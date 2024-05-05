#include "callback.h"
#include "json.hpp"
namespace fs = std::filesystem;

const char g_end_char = '\0';
const unsigned char g_empty_char = 0xFF;

int g_count = 0;
int g_reconnect = 0;

using endpoint_t = asio::ip::tcp::endpoint;

void on_read(error_code_t _error, std::size_t _bytes, std::shared_ptr<Connection> _connection)
{
    auto& conn = *_connection;
    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    auto& socket = conn.socket;
    auto& buffer = conn.buffer;
    auto& offset = conn.offset;

    SPDLOG_INFO("{} _bytes: {}", get_info(conn), _bytes);
    if (_bytes > 0)
    {
        offset += _bytes;
    }

    if (offset == 0 || buffer[offset - 1] == g_end_char)
    {
        g_count++;
        SPDLOG_INFO("{} : {}", get_info(conn), reinterpret_cast<const char*>(buffer.data()));
    }
    else
    {
        buffer.resize(offset + 4, g_empty_char);
        auto buf = asio::buffer(buffer) += offset;
        socket.async_read_some(buf, callback(_connection, on_read));
    }
}

void on_write(error_code_t _error, std::size_t _bytes, std::shared_ptr<Connection> _connection)
{
    auto& conn = *_connection;
    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    auto& socket = conn.socket;
    auto& buffer = conn.buffer;
    auto& offset = conn.offset;

    SPDLOG_INFO("{} _bytes: {}", get_info(conn), _bytes);
    if (_bytes > 0)
    {
        offset += _bytes;
    }

    if (offset == buffer.size())
    {
        SPDLOG_INFO("{} write fininish!", get_info(conn));

        offset = 0;
        buffer.resize(offset + 4, g_empty_char);
        auto buf = asio::buffer(buffer) += offset;
        socket.async_read_some(buf, callback(_connection, on_read));
    }
    else
    {
        auto buf = asio::buffer(buffer) += offset;
        socket.async_write_some(buf, callback(_connection, on_write));
    }
}

void on_connect(error_code_t _error, std::shared_ptr<Connection> _connection, endpoint_t& _endpoint)
{
    auto& conn = *_connection;
    auto& socket = conn.socket;
    if (_error)
    {
        SPDLOG_WARN("_error: {}", _error.message());
        g_reconnect++;
        auto connect_cb = [_connection, &_endpoint](error_code_t _error)
        { on_connect(_error, _connection, _endpoint); };
        socket.async_connect(_endpoint, connect_cb);
        return;
    }

    auto& buffer = conn.buffer;
    auto& offset = conn.offset;

    offset = 0;
    buffer = to_buffer("client");
    auto buf = asio::buffer(buffer) += offset;
    socket.async_write_some(buf, callback(_connection, on_write));
}

int main(int _argc, char* _argv[])
{
    (void)_argc;
    (void)_argv;

    auto config_file = fs::current_path().parent_path() / "config.json";

    auto config = config::from_path(config_file);

    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    config::get(config, "log", config::is_string, log_format);
    spdlog::set_pattern(log_format);

    std::uint16_t port = 8888;
    config::get(config, "port", config::is_number, port);

    std::string ip = "127.0.0.1";
    config::get(config, "ip", config::is_string, ip);

    int connection_number = 2;
    config::get(config, "connection_number", config::is_number, connection_number);

    asio::io_context io_context;

    endpoint_t endpoint(asio::ip::make_address(ip), port);

    for (int id = 0; id != connection_number; ++id)
    {
        auto i = std::make_shared<Connection>(io_context);
        i->id = id;
        auto connect_cb = [=, &endpoint](error_code_t _error) { on_connect(_error, i, endpoint); };
        i->socket.async_connect(endpoint, connect_cb);
    }

    io_context.run();

    SPDLOG_INFO("\ng_count: {}\ng_reconnect: {}", g_count, g_reconnect);

    return 0;
}
