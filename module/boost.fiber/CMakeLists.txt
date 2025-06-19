add_library(boost_fiber_module)
target_sources(boost_fiber_module
  PRIVATE FILE_SET CXX_MODULES FILES
    boost.fiber.ixx)
target_link_libraries(boost_fiber_module
  PUBLIC
    Boost::headers
    Boost::context
    Boost::fiber
    Boost::intrusive::module
)
target_compile_features(boost_fiber_module PRIVATE cxx_std_23)
add_library(Boost::fiber::module ALIAS boost_fiber_module)

option(USE_BOOST_FIBER_MODULE "使用boost.fiber模塊" OFF)

add_library(boost_fiber_module_interface INTERFACE)
target_compile_definitions(boost_fiber_module_interface
  INTERFACE
    USE_BOOST_FIBER_MODULE=$<BOOL:${USE_BOOST_FIBER_MODULE}>
)
target_link_libraries(boost_fiber_module_interface
  INTERFACE
    $<$<BOOL:${USE_BOOST_FIBER_MODULE}>:Boost::fiber::module;Boost::intrusive::module>
    Boost::fiber
)
add_library(Boost::fiber::module::option ALIAS boost_fiber_module_interface)
