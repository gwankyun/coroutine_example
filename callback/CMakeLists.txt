set(callback callback)
set(CMAKE_FOLDER ${callback})

set(callback_required callback_required)
add_library(${callback_required} INTERFACE)
target_sources(${callback_required}
  INTERFACE
  ${CMAKE_SOURCE_DIR}/include/connection.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/callback.h
)
target_include_directories(${callback_required}
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(${callback_required} INTERFACE
  Boost::headers
  spdlog::spdlog
  nlohmann_json::nlohmann_json
)
target_compile_features(${callback_required} INTERFACE cxx_std_23)

set(callback_server callback_server)
add_executable(${callback_server})
target_sources(${callback_server}
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/callback_server.cpp
)
target_link_libraries(${callback_server}
  PRIVATE
  ${callback_required}
)

set(callback_client callback_client)
add_executable(${callback_client})
target_sources(${callback_client}
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/callback_client.cpp
)
target_link_libraries(${callback_client}
  PRIVATE
  ${callback_required}
)

# set(callback_required callback_required)
# add_library(${callback_required} INTERFACE)
# target_sources(${callback_required} INTERFACE
# ${CMAKE_SOURCE_DIR}/include/log.hpp
# ${CMAKE_CURRENT_SOURCE_DIR}/include/common.h
# ${CMAKE_CURRENT_SOURCE_DIR}/include/data.hpp
# ${CMAKE_CURRENT_SOURCE_DIR}/src/common.cpp
# )
# target_compile_features(${callback_required} INTERFACE cxx_std_23)
# target_include_directories(${callback_required}
# INTERFACE
# ${CMAKE_SOURCE_DIR}/include
# ${CMAKE_CURRENT_SOURCE_DIR}/include
# )
# target_link_libraries(${callback_required} INTERFACE
# Boost::headers
# spdlog::spdlog
# )

# set(server_callback server_callback)
# add_executable(${server_callback})
# target_sources(${server_callback}
# PRIVATE
# ${CMAKE_SOURCE_DIR}/src/server/callback.cpp
# )
# target_link_libraries(${server_callback}
# PRIVATE
# ${callback_required}
# )

# set(client_callback client_callback)
# add_executable(${client_callback})
# target_sources(${client_callback}
# PRIVATE
# ${CMAKE_SOURCE_DIR}/src/client/callback.cpp
# )
# target_link_libraries(${client_callback}
# PRIVATE
# ${callback_required}
# )
