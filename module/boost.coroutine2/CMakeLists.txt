add_library(boost_coroutine2_module)
target_sources(boost_coroutine2_module
  PRIVATE FILE_SET CXX_MODULES FILES
    boost.coroutine2.ixx)
target_link_libraries(boost_coroutine2_module PUBLIC Boost::headers Boost::context)
target_compile_features(boost_coroutine2_module PRIVATE cxx_std_23)
add_library(Boost::coroutine2::module ALIAS boost_coroutine2_module)

option(USE_BOOST_COROUTINE2_MODULE "使用boost.coroutine2模塊" ON)

add_library(boost_coroutine2_module_interface INTERFACE)
target_compile_definitions(boost_coroutine2_module_interface
  INTERFACE
    USE_BOOST_COROUTINE2_MODULE=$<BOOL:${USE_BOOST_COROUTINE2_MODULE}>
)
target_link_libraries(boost_coroutine2_module_interface
  INTERFACE
    $<$<BOOL:${USE_BOOST_COROUTINE2_MODULE}>:Boost::coroutine2::module>
    Boost::coroutine2
)
add_library(Boost::coroutine2::module::option ALIAS boost_coroutine2_module_interface)
