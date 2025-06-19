add_library(boost_intrusive_module)
target_sources(boost_intrusive_module
  PRIVATE FILE_SET CXX_MODULES FILES
    boost.intrusive.ixx
    boost.intrusive.link_mode.ixx
    boost.intrusive.detail.ixx
)
target_include_directories(boost_intrusive_module INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(boost_intrusive_module
  PUBLIC Boost::intrusive)
add_library(Boost::intrusive::module ALIAS boost_intrusive_module)
target_compile_features(boost_intrusive_module PRIVATE cxx_std_23)

option(USE_BOOST_INTRUSIVE_MODULE "使用boost.core模塊" ON)
add_library(boost_intrusive_module_interface INTERFACE)
target_compile_definitions(boost_intrusive_module_interface
  INTERFACE
    USE_BOOST_INTRUSIVE_MODULE=$<BOOL:${USE_BOOST_INTRUSIVE_MODULE}>
)
target_link_libraries(boost_intrusive_module_interface
  INTERFACE
    $<IF:$<BOOL:${USE_BOOST_INTRUSIVE_MODULE}>,Boost::intrusive::module,Boost::intrusive>
)
add_library(Boost::intrusive::module::option ALIAS boost_intrusive_module_interface)
