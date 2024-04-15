#pragma once
#include "connection.h"

using callback_t = void (*)(
    error_code_t _error, std::size_t _bytes,
    std::shared_ptr<Connection> _connection);

auto callback =
    [](std::shared_ptr<Connection> _connection, callback_t _cb)
{
    return [_connection, _cb](error_code_t _error, std::size_t _bytes)
    {
        _cb(_error, _bytes, _connection);
    };
};
