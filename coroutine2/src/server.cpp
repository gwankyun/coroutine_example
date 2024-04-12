#include "log.hpp"
#include <spdlog/fmt/bin_to_hex.h>
#include "connection.h"
#define BOOST_ALL_NO_LIB 1
#include <boost/coroutine2/all.hpp>
#include <format>

namespace coro2 = boost::coroutines2;

struct Coro2Yield
{
    struct YieldType
    {
        using push_type = coro2::coroutine<void>::push_type;
        YieldType(std::function<void()> &_run, push_type *&_yield)
            : m_run(_run), m_push(_yield) {}
        ~YieldType() = default;

        void operator()(push_type &_push)
        {
            m_push = &_push;
            while (true)
            {
                _push(); // 響應子協程的拉（pull）動作
                m_run(); // 開始跑實際任務
            }
        }

        void push()
        {
            if (m_push != nullptr)
            {
                (*m_push)();
            }
        }

    private:
        std::function<void()> &m_run;
        push_type *&m_push;
    };

    using push_type = YieldType;
    using pull_type = coro2::coroutine<void>::pull_type;
    Coro2Yield(std::function<void()> _run)
        : m_run(std::move(_run)), yield(m_run, m_yield) {}
    ~Coro2Yield() = default;
    push_type yield;

private:
    push_type::push_type *m_yield = nullptr;
    std::function<void()> m_run;
};

inline std::string get_info(const socket_t &_data)
{
    auto remote_endpoint = _data.remote_endpoint();
    auto ip = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    return std::format("({} : {})", ip, port);
}

asio::io_context io_context;

void func(coro2::coroutine<void>::push_type &_push)
{
    SPDLOG_INFO("");
    while (true)
    {
        io_context.run();
        _push();
    }
}

    // std::vector<char> buffer;

bool read_done(std::vector<char> &buffer, std::size_t offset)
{
    SPDLOG_INFO("size: {}", buffer.size());
    SPDLOG_INFO("offset: {}", offset);
    if (offset == 0)
    {
        return false;
    }

    if (buffer.size() >= offset)
    {
        SPDLOG_INFO("{}", spdlog::to_hex(buffer));
        SPDLOG_INFO("{}", offset - 1);
        // SPDLOG_INFO("{}", *(buffer.begin() + offset - 1));
        if (buffer.size() > (offset - 1))
        {
            SPDLOG_INFO("{}", *(buffer.begin() + (offset - 1)));
        }
        return buffer.at(offset - 1) == '.';
    }
    else
    {
        return false;
    }
}

void coro_main()
{
    SPDLOG_INFO("");
    // asio::io_context io_context;
    using asio::ip::tcp;

    acceptor_t acceptor(
        io_context,
        tcp::endpoint(tcp::v4(), 8888));

    auto socket = std::make_shared<socket_t>(io_context);

    error_code_t error;
    std::size_t bytes;
    std::vector<char> buffer;
    std::size_t offset = 0;

    auto cb = [&](error_code_t _error, std::size_t _bytes = 0U)
    {
        SPDLOG_INFO("");
        error = _error;
        bytes = _bytes;
    };

    acceptor.async_accept(*socket, cb);

    SPDLOG_INFO("");
    coro2::coroutine<void>::pull_type pull(func);

    SPDLOG_INFO("");
    pull();

    if (error)
    {
        SPDLOG_WARN("{} _error: {}", get_info(*socket), error.message());
    }

    SPDLOG_INFO("{}", get_info(*socket));

    offset = 0;
    // while (buffer.empty() || buffer[offset - 1] != '.')
    do
    {
        buffer.resize(offset + 4, '\0');
        socket->async_read_some(asio::buffer(buffer.data(), buffer.size()) += offset, cb);
        pull();
        SPDLOG_INFO("{}", spdlog::to_hex(buffer));

        if (error)
        {
            SPDLOG_WARN("{} _error: {}", get_info(*socket), error.message());
            return;
        }

        if (bytes >= 0)
        {
            SPDLOG_INFO("{} _bytes: {}", get_info(*socket), bytes);
            offset += bytes;
        }
        // } while (!read_done(buffer, offset));
        SPDLOG_INFO("{}", offset);
    } while (offset == 0 || (buffer.size() >= offset && buffer[offset - 1] != '.'));

    SPDLOG_INFO("{} : {}", get_info(*socket), buffer.data());

    // io_context.run();
}

void coro2_main(
    asio::io_context &io_context,
    std::shared_ptr<Coro2Yield> coroYield,
    // Coro2Yield::push_type &coro2_yield,
    acceptor_t &acceptor,
    std::shared_ptr<socket_t> _socket)
{
    // coro2_yield()是主協程，推（push）表示將控制權推到子協程
    // 子協程即當前函數，拉（pull）表示將控制權從子協程拉回到主協程
    auto coro2_yield = coroYield->yield;
    Coro2Yield::pull_type pull(coro2_yield);
    // Coro2Yield::pull_type pull(coroYield->yield);
    SPDLOG_INFO("after yield");
    auto& socket = *_socket;
    error_code_t error;
    std::size_t bytes;
    std::size_t offset = 0;
    std::vector<char> buffer;

    auto yield = [&, _socket](error_code_t _error, std::size_t _bytes = 0U)
    {
        error = _error;
        bytes = _bytes;
        // SPDLOG_INFO("before yield");
        coro2_yield.push();
    };

    acceptor.async_accept(socket, yield);
    pull();

    if (error)
    {
        SPDLOG_INFO(error.message());
        return;
    }

    SPDLOG_INFO("{}", get_info(socket));

    auto newSocket = std::make_shared<socket_t>(acceptor.get_executor());
    // auto coro2_yield2 = std::make_shared<Coro2Yield>([&io_context]
    //                                                  { io_context.run(); });
    coro2_main(io_context, coroYield, acceptor, newSocket);

    offset = 0;
    while (offset == 0 || buffer[offset - 1] != '.')
    {
        buffer.resize(offset + 4, '\0');
        socket.async_read_some(asio::buffer(buffer) += offset, yield);
        pull();

        if (error)
        {
            SPDLOG_WARN("{} _error: {}", get_info(socket), error.message());
            return;
        }

        if (bytes >= 0)
        {
            SPDLOG_INFO("{} _bytes: {}", get_info(socket), bytes);
            offset += bytes;
        }
    }

    SPDLOG_INFO("{} : {}", get_info(socket), buffer.data());

    offset = 0;
    buffer = to_buffer("server.");
    while (offset != buffer.size())
    {
        socket.async_write_some(asio::buffer(buffer) += offset, yield);
        pull();

        if (error)
        {
            SPDLOG_WARN("{} _error: {}", get_info(socket), error.message());
            return;
        }

        if (bytes >= 0)
        {
            offset += bytes;
        }
    }

    SPDLOG_INFO("{} write fininish!", get_info(socket));
}

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v");

    // asio::io_context io_context;
    // using asio::ip::tcp;

    // acceptor_t acceptor(
    //     io_context,
    //     tcp::endpoint(tcp::v4(), 8888));

    // // auto data = std::make_shared<TcpConnection>(io_context);
    // // accept_socket(error_code_t{}, 0U, acceptor, data);

    // auto socket = std::make_shared<socket_t>(io_context);

    // // Coro2Yield coro2_yield(
    // //     [&]
    // //     {
    // //         io_context.run();
    // //     });
    // auto coro2_yield = std::make_shared<Coro2Yield>([&]
    //                                                 { io_context.run(); });
    // // coro2_main(coro2_yield, coro2_yield->yield, acceptor, socket);
    // coro2_main(io_context, coro2_yield, acceptor, socket);

    // io_context.run();

    coro_main();

    return 0;
}
