#include "callback.h"
#include "json.hpp"
namespace fs = std::filesystem;

struct Global
{
    const char end_char = '\0';
    const unsigned char empty_char = 0xFF;
    int wait_seconds = 3;
} global;

template <typename T>
using sptr = std::shared_ptr<T>;

using steady_timer_t = asio::steady_timer;

void on_write(error_code_t _error, std::size_t _bytes, sptr<Connection> _connection);

void on_read(error_code_t _error, std::size_t _bytes, sptr<Connection> _connection)
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

    if (offset == 0 || buffer[offset - 1] == global.end_char)
    {
        SPDLOG_INFO("{} : {}", get_info(conn), reinterpret_cast<const char*>(buffer.data()));

        asio::chrono::seconds t(global.wait_seconds);
        auto timer = std::make_shared<steady_timer_t>(socket.get_executor(), t);
        timer->async_wait(
            [timer, &conn, _connection, &buffer, &offset, &socket](error_code_t _error)
            {
                if (_error)
                {
                    SPDLOG_INFO("{}: {}", get_info(conn), _error.message());
                    return;
                }

                SPDLOG_INFO("{} timeout!", get_info(conn));
                buffer.resize(offset);
                offset = 0;
                auto buf = asio::buffer(buffer) += offset;
                socket.async_write_some(buf, callback(_connection, on_write));
            });
    }
    else
    {
        buffer.resize(offset + 4, global.empty_char);
        auto buf = asio::buffer(buffer) += offset;
        socket.async_read_some(buf, callback(_connection, on_read));
    }
}

void on_write(error_code_t _error, std::size_t _bytes, sptr<Connection> _connection)
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
        buffer.resize(offset + 4, global.empty_char);
        auto buf = asio::buffer(buffer) += offset;
        socket.async_read_some(buf, callback(_connection, on_read));
    }
    else
    {
        auto buf = asio::buffer(buffer) += offset;
        socket.async_write_some(buf, callback(_connection, on_write));
    }
}

void on_accept(error_code_t _error, acceptor_t& _acceptor, sptr<Connection> _connection)
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
    auto& executor = _acceptor.get_executor();

    auto new_conn = std::make_shared<Connection>(executor);
    auto accept_cb = [new_conn, &_acceptor](error_code_t error) { on_accept(error, _acceptor, new_conn); };
    _acceptor.async_accept(new_conn->socket, accept_cb);

    SPDLOG_INFO("{}", get_info(conn));

    offset = 0;
    buffer.resize(offset + 4, global.empty_char);
    auto buf = asio::buffer(buffer) += offset;
    socket.async_read_some(buf, callback(_connection, on_read));
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

    asio::io_context io_context;
    using asio::ip::tcp;

    acceptor_t acceptor(io_context, tcp::endpoint(tcp::v4(), port));

    {
        auto conn = std::make_shared<Connection>(io_context);
        auto accept_cb = [conn, &acceptor](error_code_t error) { on_accept(error, acceptor, conn); };
        acceptor.async_accept(conn->socket, accept_cb);
    }

    io_context.run();

    return 0;
}
