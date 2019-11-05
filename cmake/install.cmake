include(GNUInstallDirs)

function(install_deps_headers)
  install(DIRECTORY ${MICROSOFT.GSL_ROOT}/include/gsl
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(DIRECTORY deps/outcome/outcome
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

endfunction()

### filecoin_install should be called right after add_library(target)
function (filecoin_install targets)
  install(TARGETS ${targets} EXPORT filecoinConfig
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX}
      )
endfunction()

function(filecoin_install_setup)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs HEADER_DIRS)
  cmake_parse_arguments(arg "${options}" "${oneValueArgs}"
      "${multiValueArgs}" ${ARGN})

  foreach (DIR IN ITEMS ${arg_HEADER_DIRS})
    get_filename_component(FULL_PATH ${DIR} ABSOLUTE)
    string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/core filecoin RELATIVE_PATH ${FULL_PATH})
    get_filename_component(INSTALL_PREFIX ${RELATIVE_PATH} DIRECTORY)
    install(DIRECTORY ${DIR}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${INSTALL_PREFIX}
        FILES_MATCHING PATTERN "*.hpp")
  endforeach ()

  install(
      EXPORT filecoinTargets
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/filecoin
      NAMESPACE fc::
  )
  export(
      EXPORT filecoinTargets
      FILE ${CMAKE_CURRENT_BINARY_DIR}/filecoinTargets.cmake
      NAMESPACE fc::
  )

endfunction()
