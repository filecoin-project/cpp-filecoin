#.rst:
# Findfilecoin_ffi.cmake
# -----------
# Finds the Filecoin FFI library
#
# This will define the following variables:
#
#   FILECOIN_FFI_FOUND        -  system has filecoin_ffi
#   FILECOIN_FFI_INCLUDE_DIRS -  filecoin_ffi include directories
#   FILECOIN_FFI_LIBRARY      -  library needed to use filecoin_ffi
#
# and the following imported targets:
#
#   filecoin_ffi   - The filecoin_ffi library
#

find_package(PkgConfig REQUIRED)

find_path(FILECOIN_FFI_INCLUDE_DIR
  NAMES filecoin-ffi/filcrypto.h
  HINTS "${FILECOIN_FFI_ROOT_DIR}" "$ENV{FILECOIN_FFI_ROOT_DIR}"
  PATH_SUFFIXES include
)

find_library(FILECOIN_FFI_LIBRARY
  NAMES filcrypto
  HINTS "${FILECOIN_FFI_ROOT_DIR}" "$ENV{FILECOIN_FFI_ROOT_DIR}"
  PATH_SUFFIXES "${LIBRARY_PATH_PREFIX}"
)
message(FALAL_ERROR "${LIBRARY_PATH_PREFIX}")

pkg_check_modules(PKG_FILECOIN IMPORTED_TARGET filcrypto)

if (PKG_FILECOIN_FOUND)
  set(FILECOIN_FFI_FOUND "${PKG_FILECOIN_FOUND}")
  set(FILECOIN_FFI_INCLUDE_DIRS "${FILECOIN_FFI_INCLUDE_DIR}")
  set(FILECOIN_FFI_LIBRARIES "${FILECOIN_FFI_LIBRARY}")

  add_library(filecoin_ffi
    STATIC
    IMPORTED
    GLOBAL
  )

  set_target_properties(filecoin_ffi
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${FILECOIN_FFI_INCLUDE_DIR}"
      IMPORTED_LOCATION "${FILECOIN_FFI_LIBRARY}"
  )

  target_link_libraries(filecoin_ffi
    INTERFACE PkgConfig::PKG_FILECOIN
  )
endif()

mark_as_advanced(FILECOIN_FFI_INCLUDE_DIR)
mark_as_advanced(FILECOIN_FFI_LIBRARY)
