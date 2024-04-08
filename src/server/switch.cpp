#include <vector>
#include "common.h"

struct Callback
{
    Callback(socket_t &&_socket) : m_data(std::make_shared<Data>(std::move(_socket))) {}
    ~Callback() = default;
    std::shared_ptr<Data> m_data;

    void clear()
    {
        auto &data = *m_data;
        auto &offset = data.offset;
        auto &buffer = data.buffer;
        offset = 0;
        buffer.clear();
    }

    void operator()(error_code_t _error, std::size_t _bytes)
    {
        auto &data = *m_data;
        auto &socket = data.socket;
        auto &offset = data.offset;
        auto &buffer = data.buffer;
        auto &state = data.state;

        auto remote_endpoint = socket.remote_endpoint();
        auto ip = remote_endpoint.address().to_string();
        auto port = remote_endpoint.port();

        if (_error)
        {
            SPDLOG_WARN("({} : {}) _error: {}", ip, port, _error.message());
            return;
        }

        if (_bytes >= 0)
        {
            SPDLOG_INFO("({} : {}) _bytes: {}", ip, port, _bytes);
            offset += _bytes;
        }

        CORO_BEGIN(state);
        // 讀
        clear();
        do
        {
            buffer.resize(offset + 4, '\0');
            socket.async_read_some(asio::buffer(buffer.data(), buffer.size()) += offset, *this);
            CORO_YIELD(state);
        } while (buffer[offset - 1] != '.');

        SPDLOG_INFO("({} : {}) : {}", ip, port, buffer.data());

        // 寫
        clear();
        [&]
        {
            std::string str{"server."};
            std::copy_n(str.c_str(), str.size(), std::back_inserter(buffer));
        }();

        do
        {
            socket.async_write_some(asio::buffer(buffer.data(), buffer.size()) += offset, *this);
            CORO_YIELD(state);
        } while (buffer.size() != offset);

        SPDLOG_INFO("({} : {}) write fininish!", ip, port);

        CORO_END();
    }
};

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v");

    asio::io_context io_context;
    using asio::ip::tcp;

    acceptor_t acceptor(
        io_context,
        tcp::endpoint(tcp::v4(), 8888));

    {
        auto acceptor_callback = std::make_shared<Accept>(
            acceptor,
            [](AcceptData &_data)
            {
                auto &data = _data;
                auto &socket = data.socket;
                auto remote_endpoint = socket->remote_endpoint();
                auto ip = remote_endpoint.address().to_string();
                auto port = remote_endpoint.port();
                SPDLOG_INFO("({} : {})", ip, port);

                auto callback = std::make_shared<Callback>(std::move(*socket));
                (*callback)(error_code_t{}, 0UL);
            });
        (*acceptor_callback)(error_code_t{});
    }

    io_context.run();

    return 0;
}
