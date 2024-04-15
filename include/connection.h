#pragma once
#include <vector>
#include <boost/system.hpp> // boost::system::error_code
#include <boost/asio.hpp>
#include <cstddef>
#include <format>

using error_code_t = boost::system::error_code;
namespace asio = boost::asio;

using acceptor_t = asio::ip::tcp::acceptor;
using socket_t = asio::ip::tcp::socket;

using buffer_t = std::vector<unsigned char>;

template <typename T>
struct Connection
{
    Connection(asio::io_context &_io_context) : socket(_io_context) {}
    Connection(const asio::any_io_executor &_io_context) : socket(_io_context) {}
    ~Connection() = default;

    T socket;

    buffer_t buffer;
    std::size_t offset = 0;

    int state = 0;
    int per_read_size = 4; // 每次讀多長
    int id = 0;
    /// @brief 判斷是否讀完
    bool read_done()
    {
        if (offset <= 0)
        {
            return false;
        }
        return buffer[offset - 1] == '\0';
    }
};

using TcpConnection = Connection<socket_t>;

inline std::string get_info(TcpConnection& _data)
{
    auto remote_endpoint = _data.socket.remote_endpoint();
    auto ip = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    return std::format("({} : {} {})", ip, port, _data.id);
}

inline buffer_t to_buffer(std::string _str)
{
    buffer_t buf;
    std::copy_n(_str.c_str(), _str.size(), std::back_inserter(buf));
    buf.push_back('\0');
    return buf;
}

struct Argument
{
    asio::ip::port_type port = 8888;
    asio::ip::address address;
    std::size_t connect_number = 2u;

    void parse_server(int _argc, char* _argv[])
    {
        if (_argc >= 2)
        {
            this->port = asio::ip::port_type(std::stoul(_argv[1]));
        }
        SPDLOG_INFO("port: {}", port);
    }

    void parse_client(int _argc, char* _argv[])
    {
        address = asio::ip::make_address("127.0.0.1");
        if (_argc >= 2)
        {
            address = asio::ip::make_address(_argv[1]);
        }
        SPDLOG_INFO("address: {}", address.to_string());

        port = 8888;
        if (_argc >= 3)
        {
            port = asio::ip::port_type(std::stoul(_argv[2]));
        }
        SPDLOG_INFO("port: {}", port);

        connect_number = 2u;
        if (_argc >= 4)
        {
            connect_number = std::stoi(_argv[3]);
        }
        SPDLOG_INFO("conn_count: {}", connect_number);
    }
};

