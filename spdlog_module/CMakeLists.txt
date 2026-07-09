add_library(spdlog_module)
target_sources(spdlog_module
  PUBLIC
    FILE_SET CXX_MODULES
      FILES
        src/spdlog.ixx
    FILE_SET HEADERS
      FILES
        spdlog_module/spdlog.h
)
target_link_libraries(spdlog_module
  PUBLIC
    spdlog::spdlog
)
target_compile_features(spdlog_module PUBLIC cxx_std_23)

if(PROJECT_IS_TOP_LEVEL)
  include(GNUInstallDirs)

  install(
    TARGETS spdlog_module
    EXPORT spdlog_moduleTargets
    FILE_SET CXX_MODULES
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/spdlog_module/modules
    FILE_SET HEADERS
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/spdlog_module
  )

  install(
    EXPORT spdlog_moduleTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/spdlog_module
    NAMESPACE spdlog_module::
  )

  include(CMakePackageConfigHelpers)

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/spdlog_moduleConfigVersion.cmake
    COMPATIBILITY ExactVersion
  )

  install(
    FILES
      cmake/spdlog_moduleConfig.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/spdlog_moduleConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/spdlog_module
  )
endif()
