add_library(spdlog_module)
target_sources(spdlog_module
  PRIVATE
    spdlog/compat.h
  PRIVATE FILE_SET CXX_MODULES FILES
    spdlog.ixx)
target_include_directories(spdlog_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(spdlog_module PUBLIC spdlog::spdlog)
target_compile_features(spdlog_module PRIVATE cxx_std_23)
add_library(spdlog::module ALIAS spdlog_module)
