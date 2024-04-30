#pragma once
#include <nlohmann/json.hpp>
#include <filesystem>
#include <utility>
#include <fstream>
#include <iostream>

namespace config
{
    using json = nlohmann::json;

    inline json from_path(const std::filesystem::path &_path)
    {
        config::json config;
        if (std::filesystem::exists(_path))
        {
            std::ifstream config_content(_path);
            config = config::json::parse(config_content);
        }
        return std::move(config);
    }

    inline bool is_string(const json& _config)
    {
        return _config.is_string();
    }

    inline bool is_number(const json& _config)
    {
        return _config.is_number();
    }

    using checker_t = std::function<bool(const json&)>;

    template <typename T>
    inline bool get(const json& _config, const std::string _key, checker_t _checker, T& _value)
    {
        if (_config.contains(_key))
        {
            auto value = _config[_key];
            if (_checker(value))
            {
                _value = value.template get<T>();
                return true;
            }
        }
        return false;
    }
} // namespace config
