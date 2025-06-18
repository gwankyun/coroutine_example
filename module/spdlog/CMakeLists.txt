add_library(spdlog_module)
target_sources(spdlog_module
  PRIVATE
    spdlog/spdlog.h
  PRIVATE FILE_SET CXX_MODULES FILES
    spdlog.ixx)
target_include_directories(spdlog_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(spdlog_module PUBLIC spdlog::spdlog)
target_compile_features(spdlog_module PRIVATE cxx_std_23)
add_library(spdlog::module ALIAS spdlog_module)

option(USE_SPDLOG_MODULE "使用spdlog模塊" ON)

add_library(spdlog_module_interface INTERFACE)
target_include_directories(spdlog_module_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(spdlog_module_interface
  INTERFACE
    USE_SPDLOG_MODULE=$<BOOL:${USE_SPDLOG_MODULE}>
)
target_link_libraries(spdlog_module_interface
  INTERFACE
    $<$<BOOL:${USE_SPDLOG_MODULE}>:spdlog::module>
    spdlog::spdlog
)
add_library(spdlog::module::option ALIAS spdlog_module_interface)
