#include "log.hpp"
#include <vector>
#include <boost/system.hpp> // boost::system::error_code
#include <boost/asio.hpp>
#include <cstddef>

using error_code_t = boost::system::error_code;
namespace asio = boost::asio;

using acceptor_t = asio::ip::tcp::acceptor;
using socket_t = asio::ip::tcp::socket;

struct DataBase
{
    DataBase() = default;
    virtual ~DataBase() = default;
    using buffer_t = std::vector<char>;
    buffer_t buffer;
    std::size_t offset = 0;
};

template <typename T>
struct Data : public DataBase
{
    Data(T &&_socket) : socket(std::move(_socket)) {}
    ~Data() override = default;
    T socket;

    static Data &from(std::shared_ptr<DataBase> _base)
    {
        return *std::dynamic_pointer_cast<Data>(_base);
    }
};

using SocketData = Data<socket_t>;

void on_write(error_code_t _error, std::size_t _bytes, std::shared_ptr<DataBase> _data);

void on_read(error_code_t _error, std::size_t _bytes, std::shared_ptr<DataBase> _data)
{
    auto &data = SocketData::from(_data);
    auto &socket = data.socket;
    auto remote_endpoint = socket.remote_endpoint();
    auto ip = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    if (_error)
    {
        SPDLOG_WARN("({} : {}) _error: {}", ip, port, _error.message());
        return;
    }

    auto &buffer = data.buffer;
    auto &offset = data.offset;

    SPDLOG_INFO("({} : {}) _bytes: {}", ip, port, _bytes);
    offset += _bytes;

    if (buffer[offset - 1] == '.')
    {
        SPDLOG_INFO("({} : {}) : {}", ip, port, buffer.data());
        std::string str{"server."};

        buffer.clear();
        offset = 0;
        std::copy_n(str.c_str(), str.size(), std::back_inserter(buffer));
        socket.async_write_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            [_data](error_code_t _error, std::size_t _bytes)
            {
                on_write(_error, _bytes, _data);
            });
    }
    else
    {
        buffer.resize(offset + 4, '\0');
        socket.async_read_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            [_data](error_code_t _error, std::size_t _bytes)
            {
                on_read(_error, _bytes, _data);
            });
    }
}

void on_write(error_code_t _error, std::size_t _bytes, std::shared_ptr<DataBase> _data)
{
    auto &data = SocketData::from(_data);
    auto &socket = data.socket;
    auto &buffer = data.buffer;
    auto &offset = data.offset;
    auto remote_endpoint = socket.remote_endpoint();
    auto ip = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    if (_error)
    {
        SPDLOG_WARN("({} : {}) _error: {}", ip, port, _error.message());
        return;
    }

    SPDLOG_INFO("({} : {}) _bytes: {}", ip, port, _bytes);

    offset += _bytes;

    if (offset == buffer.size())
    {
        SPDLOG_INFO("({} : {}) write fininish!", ip, port);

        offset = 0;
        buffer.resize(offset + 4, '\0');
        socket.async_read_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            [_data](error_code_t _error, std::size_t _bytes)
            {
                on_read(_error, _bytes, _data);
            });
    }
    else
    {
        socket.async_write_some(
            asio::buffer(buffer.data(), buffer.size()) += offset,
            [_data](error_code_t _error, std::size_t _bytes)
            {
                on_write(_error, _bytes, _data);
            });
    }
}

void callback(std::shared_ptr<DataBase> _data)
{
    auto &data = SocketData::from(_data);
    auto &socket = data.socket;
    auto &buffer = data.buffer;
    auto &offset = data.offset;

    offset = 0;

    buffer.resize(offset + 4, '\0');
    socket.async_read_some(
        asio::buffer(buffer.data(), buffer.size()) += offset,
        [_data](error_code_t _error, std::size_t _bytes)
        {
            on_read(_error, _bytes, _data);
        });
}

void on_accept(
    error_code_t _error,
    acceptor_t &_acceptor,
    std::shared_ptr<socket_t> _socket)
{
    if (_error)
    {
        SPDLOG_WARN(DBG(_error.message()));
        return;
    }

    auto newSocket = std::make_shared<socket_t>(_acceptor.get_executor());
    _acceptor.async_accept(
        *newSocket,
        [newSocket, &_acceptor](error_code_t error)
        {
            on_accept(error, _acceptor, newSocket);
        });

    auto remote_endpoint = _socket->remote_endpoint();
    auto ip = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    SPDLOG_INFO("({} : {})", ip, port);

    callback(std::make_shared<SocketData>(std::move(*_socket))); // 回調版本
}

int main()
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v");

    asio::io_context io_context;
    using asio::ip::tcp;

    acceptor_t acceptor(
        io_context,
        tcp::endpoint(tcp::v4(), 8888));

    {
        auto socket = std::make_shared<socket_t>(io_context);
        acceptor.async_accept(
            *socket,
            [socket, &acceptor](error_code_t error)
            {
                on_accept(error, acceptor, socket);
            });
    }

    io_context.run();

    return 0;
}
