#pragma once
#include "common.h"
#include <algorithm>
#include <memory> // std::shared_ptr
#define BOOST_ASIO_NO_DEPRECATED 1
#include <boost/asio.hpp>   // boost::asio
// #include <boost/system.hpp> // boost::system::error_code
#include "log.hpp"

using acceptor_t = asio::ip::tcp::acceptor;
using socket_t = asio::ip::tcp::socket;

template <typename T>
struct Data : public ConnectionBase
{
    Data(T &&_socket) : socket(std::move(_socket)) {}
    ~Data() override = default;
    T socket;

    static Data& from(std::shared_ptr<ConnectionBase> _base)
    {
        return *std::dynamic_pointer_cast<Data>(_base);
    }
};

using SocketData = Data<socket_t>;
