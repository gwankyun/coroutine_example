add_library(boost_scope_module)
target_sources(boost_scope_module
  PRIVATE
    boost/scope/defer.hpp
  PRIVATE FILE_SET CXX_MODULES FILES
    boost.scope.ixx)
target_include_directories(boost_scope_module INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(boost_scope_module
  PUBLIC Boost::scope Boost::config)
target_compile_features(boost_scope_module PRIVATE cxx_std_23)
add_library(Boost::scope::module ALIAS boost_scope_module)

option(USE_BOOST_SCOPE_MODULE "使用boost.scope模塊" ON)
add_library(boost_scope_module_interface INTERFACE)
target_compile_definitions(boost_scope_module_interface
  INTERFACE
    USE_BOOST_SCOPE_MODULE=$<BOOL:${USE_BOOST_SCOPE_MODULE}>
)
target_link_libraries(boost_scope_module_interface
  INTERFACE
    $<IF:$<BOOL:${USE_BOOST_SCOPE_MODULE}>,Boost::scope::module,Boost::scope>
)
add_library(Boost::scope::module::option ALIAS boost_scope_module_interface)
