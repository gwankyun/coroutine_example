cmake_print_variables(CMAKE_SOURCE_DIR)
cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)

set(coroutine2 coroutine2)

set(coroutine2_required coroutine2_required)
add_library(${coroutine2_required} INTERFACE)
target_sources(${coroutine2_required}
  INTERFACE
    ${CMAKE_SOURCE_DIR}/include/connection.h
)
target_include_directories(${coroutine2_required}
  INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(${coroutine2_required} INTERFACE
  Boost::headers
  Boost::context
  spdlog::spdlog
)
target_compile_features(${coroutine2_required} INTERFACE cxx_std_23)

set(coroutine2_server coroutine2_server)
add_executable(${coroutine2_server})
target_sources(${coroutine2_server}
  PRIVATE
    src/coroutine2_server.cpp
)
target_link_libraries(${coroutine2_server}
  PRIVATE
    ${coroutine2_required}
)
set_target_properties(${coroutine2_server} PROPERTIES FOLDER ${coroutine2})

set(boost_coro2 boost_coro2)
add_executable(${boost_coro2})
target_sources(${boost_coro2} PRIVATE
  src/coro2.cpp
)
target_link_libraries(${boost_coro2}
  PRIVATE
    ${coroutine2_required}
)
set_target_properties(${boost_coro2} PROPERTIES FOLDER ${coroutine2})
