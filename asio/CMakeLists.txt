set(asio_callback asio_callback)
add_executable(${asio_callback})
target_sources(${asio_callback} PRIVATE
  src/asio_callback.cpp
)
target_link_libraries(${asio_callback} PRIVATE
  Boost::headers
  spdlog::spdlog
)

set(asio_switch asio_switch)
add_executable(${asio_switch})
target_sources(${asio_switch} PRIVATE
  src/asio_switch.cpp
)
target_link_libraries(${asio_switch} PRIVATE
  Boost::headers
  spdlog::spdlog
)

set(asio_coroutine2 asio_coroutine2)
add_executable(${asio_coroutine2})
target_sources(${asio_coroutine2} PRIVATE
  src/asio_coroutine2.cpp
)
target_link_libraries(${asio_coroutine2} PRIVATE
  Boost::headers
  spdlog::spdlog
  Boost::context
)

set(asio_continuatioin asio_continuatioin)
add_executable(${asio_continuatioin})
target_sources(${asio_continuatioin} PRIVATE
  src/asio_continuatioin.cpp
)
target_link_libraries(${asio_continuatioin} PRIVATE
  Boost::headers
  spdlog::spdlog
  Boost::context
)

set(asio_fiber asio_fiber)
add_executable(${asio_fiber})
target_sources(${asio_fiber} PRIVATE
  src/asio_fiber.cpp
)
target_link_libraries(${asio_fiber} PRIVATE
  Boost::headers
  spdlog::spdlog
  Boost::context
)
