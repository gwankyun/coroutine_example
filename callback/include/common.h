#pragma once
#include <vector>
#include <boost/system.hpp> // boost::system::error_code

using error_code_t = boost::system::error_code;

struct DataBase
{
    DataBase() = default;
    virtual ~DataBase() = default;
    using buffer_t = std::vector<char>;
    buffer_t buffer;
    std::size_t offset = 0;
};

void on_read(error_code_t _error, std::size_t _bytes, std::shared_ptr<DataBase> _data);
void on_write(error_code_t _error, std::size_t _bytes, std::shared_ptr<DataBase> _data);
