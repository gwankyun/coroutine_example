add_library(boost_context_module)
target_sources(boost_context_module
  PRIVATE FILE_SET CXX_MODULES FILES
    boost.context.ixx)
target_link_libraries(boost_context_module PUBLIC Boost::headers Boost::context)
target_compile_features(boost_context_module PRIVATE cxx_std_23)
add_library(Boost::context::module ALIAS boost_context_module)

option(USE_BOOST_CONTEXT_MODULE "使用boost.context模塊" ON)

add_library(boost_context_module_interface INTERFACE)
target_compile_definitions(boost_context_module_interface
  INTERFACE
    USE_BOOST_CONTEXT_MODULE=$<BOOL:${USE_BOOST_CONTEXT_MODULE}>
)
target_link_libraries(boost_context_module_interface
  INTERFACE
    $<$<BOOL:${USE_BOOST_CONTEXT_MODULE}>:Boost::context::module>
    Boost::context
)
add_library(Boost::context::module::option ALIAS boost_context_module_interface)
