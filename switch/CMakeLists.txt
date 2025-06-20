set(switch switch)

set(switch_required switch_required)
add_library(${switch_required} INTERFACE)
target_sources(${switch_required} INTERFACE
  ${CMAKE_SOURCE_DIR}/include/connection.h
)
target_include_directories(${switch_required}
  INTERFACE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${switch_required} INTERFACE
  Boost::headers
  spdlog::spdlog
  nlohmann_json::nlohmann_json
)
target_compile_features(${switch_required} INTERFACE cxx_std_23)

add_executable(switch_server)
target_sources(switch_server
  PRIVATE FILE_SET CXX_MODULES FILES
    src/switch_server.ixx
)
target_link_libraries(switch_server
  PRIVATE
    ${switch_required}
    spdlog::module::option
)
set_target_properties(switch_server PROPERTIES FOLDER ${switch})
target_compile_features(switch_server PRIVATE cxx_std_23)
