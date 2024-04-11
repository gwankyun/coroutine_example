#include <vector>
#include <boost/system.hpp> // boost::system::error_code
#include <boost/asio.hpp>
#include <cstddef>
#include <format>

using error_code_t = boost::system::error_code;
namespace asio = boost::asio;

using acceptor_t = asio::ip::tcp::acceptor;
using socket_t = asio::ip::tcp::socket;

struct ConnectionBase
{
    ConnectionBase() = default;
    virtual ~ConnectionBase() = default;
    using buffer_t = std::vector<char>;
    buffer_t buffer;
    std::size_t offset = 0;
};

template <typename T>
struct Connection : public ConnectionBase
{
    Connection(asio::io_context &_io_context) : socket(_io_context) {}
    Connection(const asio::any_io_executor &_io_context) : socket(_io_context) {}
    ~Connection() override = default;
    T socket;
    int state = 0;
    int per_read_size = 4; // 每次讀多長
    /// @brief 判斷是否讀完
    bool read_done()
    {
        if (offset <= 0)
        {
            return false;
        }
        return buffer[offset - 1] == '.';
    }

    buffer_t get_write_buffer()
    {
        std::string str{"server."};
        buffer_t buf;
        std::copy_n(str.c_str(), str.size(), std::back_inserter(buf));
        return buf;
    }

    /// @brief 基類轉引用
    static Connection &from(std::shared_ptr<ConnectionBase> _base)
    {
        return *std::dynamic_pointer_cast<Connection>(_base);
    }
};

using TcpConnection = Connection<socket_t>;

inline std::string get_info(TcpConnection& _data)
{
    auto remote_endpoint = _data.socket.remote_endpoint();
    auto ip = remote_endpoint.address().to_string();
    auto port = remote_endpoint.port();
    return std::format("({} : {})", ip, port);
}
