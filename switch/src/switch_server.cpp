#include "connection.h"
#include "json.hpp"
#include "log.hpp"
namespace fs = std::filesystem;

// clang-format off
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
// clang-format on

struct Global
{
    const char end_char = '\0';
    const unsigned char empty_char = 0xFF;
    int wait_seconds = 3;
} global;

template <typename T>
using sptr = std::shared_ptr<T>;

using steady_timer_t = asio::steady_timer;

void handle_connection(error_code_t _error, std::size_t _bytes, acceptor_t& _acceptor, sptr<steady_timer_t> _timer,
                       sptr<Connection> _connection)
{
    auto& conn = *_connection;
    auto& socket = conn.socket;
    auto& buffer = conn.buffer;
    auto& offset = conn.offset;
    auto& state = conn.state;
    auto& executor = _acceptor.get_executor();

    auto cb = [=, &_acceptor](sptr<steady_timer_t> _timer = nullptr)
    {
        return [=, &_acceptor](error_code_t _error, std::size_t _bytes = 0U)
        { handle_connection(_error, _bytes, _acceptor, _timer, _connection); };
    };

    CORO_BEGIN(state);

    _acceptor.async_accept(socket, cb());
    CORO_YIELD(state);

    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }

    SPDLOG_INFO("{}", get_info(conn));

    // switch裡不能有局部變量，所以要作特殊處理。
    [&]
    {
        auto new_conn = std::make_shared<Connection>(executor);
        handle_connection(error_code_t{}, 0U, _acceptor, _timer, new_conn);
    }();

    offset = 0;
    while (offset == 0 || buffer[offset - 1] != global.end_char)
    {
        buffer.resize(offset + 4, global.empty_char);
        socket.async_read_some(asio::buffer(buffer) += offset, cb());
        CORO_YIELD(state);

        if (_error)
        {
            SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
            return;
        }

        if (_bytes >= 0)
        {
            SPDLOG_INFO("{} _bytes: {}", get_info(conn), _bytes);
            offset += _bytes;
        }
    }

    SPDLOG_INFO("{} : {}", get_info(conn), reinterpret_cast<const char*>(buffer.data()));

    [&]
    {
        asio::chrono::seconds t(global.wait_seconds);
        _timer = std::make_shared<steady_timer_t>(socket.get_executor(), t);
    }();
    _timer->async_wait(cb(_timer));
    CORO_YIELD(state);
    if (_error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
        return;
    }
    SPDLOG_WARN("{}: timeout!", get_info(conn));

    buffer.resize(offset);
    offset = 0;
    while (offset != buffer.size())
    {
        socket.async_write_some(asio::buffer(buffer) += offset, cb());
        CORO_YIELD(state);

        if (_error)
        {
            SPDLOG_WARN("{} _error: {}", get_info(conn), _error.message());
            return;
        }

        if (_bytes >= 0)
        {
            offset += _bytes;
        }
    }

    SPDLOG_INFO("{} write fininish!", get_info(conn));

    CORO_END();
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

    namespace ip = asio::ip;
    using ip::tcp;

    asio::io_context io_context;

    acceptor_t acceptor(io_context, tcp::endpoint(tcp::v4(), port));

    {
        auto conn = std::make_shared<Connection>(io_context);
        sptr<steady_timer_t> timer;
        handle_connection(error_code_t{}, 0U, acceptor, timer, conn);
    }

    io_context.run();

    return 0;
}
