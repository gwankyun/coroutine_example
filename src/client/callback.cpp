#include "data.hpp"

void on_read(error_code_t _error, std::size_t _bytes, std::shared_ptr<ConnectionBase> _data)
{
    if (_error)
    {
        SPDLOG_WARN(DBG(_error.message()));
        return;
    }

    auto &data = SocketData::from(_data);
    auto &socket = data.socket;
    auto &buffer = data.buffer;
    auto &offset = data.offset;

    SPDLOG_INFO(DBG(_bytes));
    offset += _bytes;

    if (buffer[offset - 1] == '.' || _bytes == 0)
    {
        SPDLOG_INFO(DBG(buffer.data()));
        socket.close();
    }
    else
    {
        async_read(_data, 4, on_read);
    }
}

void callback(std::shared_ptr<ConnectionBase> _data)
{
    std::string str{"client."};
    async_write(_data, asio::buffer(str.c_str(), str.size()), on_write);
}

void on_connect(
    error_code_t _error,
    std::shared_ptr<socket_t> _socket)
{
    if (_error)
    {
        SPDLOG_WARN(DBG(_error.message()));
        return;
    }

    callback(std::make_shared<SocketData>(std::move(*_socket))); // 回調版本
}

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v");

    asio::io_context io_context;

    namespace ip = asio::ip;
    auto address = ip::make_address("127.0.0.1");
    auto endpoint = ip::tcp::endpoint(address, 8888);

    std::vector<std::shared_ptr<socket_t>> socket_vec(2);

    for (auto& socket : socket_vec)
    {
        socket = std::make_shared<socket_t>(io_context);
        socket->async_connect(
            endpoint,
            [socket](error_code_t _error)
            {
                on_connect(_error, socket);
            });
    }

    io_context.run();

    return 0;
}
