add_library(boost_asio_module)
target_sources(boost_asio_module
  PRIVATE FILE_SET CXX_MODULES FILES
    boost.asio.ixx
)
target_link_libraries(boost_asio_module PUBLIC Boost::headers Boost::asio)
target_compile_features(boost_asio_module PRIVATE cxx_std_23)
add_library(Boost::asio::module ALIAS boost_asio_module)

option(USE_BOOST_ASIO_MODULE "使用boost.asio模塊" ON)

add_library(boost_asio_module_interface INTERFACE)
target_compile_definitions(boost_asio_module_interface
  INTERFACE
    USE_BOOST_ASIO_MODULE=$<BOOL:${USE_BOOST_ASIO_MODULE}>
)
target_link_libraries(boost_asio_module_interface
  INTERFACE
    $<$<BOOL:${USE_BOOST_ASIO_MODULE}>:Boost::asio::module>
    Boost::asio
)
add_library(Boost::asio::module::option ALIAS boost_asio_module_interface)
