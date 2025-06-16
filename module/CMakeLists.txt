set(CMAKE_FOLDER module)

add_subdirectory(catch2)

add_subdirectory(spdlog)

add_subdirectory(asio)

add_subdirectory(boost.asio)

add_subdirectory(boost.scope)

add_subdirectory(boost.context)

add_subdirectory(boost.coroutine2)

add_subdirectory(boost.fiber)

add_executable(asio_module_example)
target_sources(asio_module_example
  PRIVATE FILE_SET CXX_MODULES FILES
    src/asio_module_example.ixx
)
target_link_libraries(asio_module_example PUBLIC asio::module)
target_compile_features(asio_module_example PRIVATE cxx_std_23)

add_executable(boost_asio_module_example)
target_sources(boost_asio_module_example
  PRIVATE FILE_SET CXX_MODULES FILES
    src/boost.asio.example.ixx
)
target_link_libraries(boost_asio_module_example PUBLIC Boost::asio::module)
target_compile_features(boost_asio_module_example PRIVATE cxx_std_23)
