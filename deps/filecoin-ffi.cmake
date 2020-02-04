#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

set(FILECOIN_FFI_PATH "${PROJECT_SOURCE_DIR}/deps/filecoin-ffi")
set(FILECOIN_FFI_INCLUDES
        "${FILECOIN_FFI_PATH}/include/filecoin-ffi"
        )
set(FILECOIN_FFI_LIB
        "${FILECOIN_FFI_PATH}/libfilecoin.a")

file(MAKE_DIRECTORY ${FILECOIN_FFI_INCLUDES})
set(ENV{FFI_BUILD_FROM_SOURCE} "1")

add_custom_target(
        filecoin_ffi_build
        COMMAND make
        WORKING_DIRECTORY ${FILECOIN_FFI_PATH}
)

add_custom_target(
        filecoin_ffi_fix_include
        COMMAND mkdir -p ${FILECOIN_FFI_INCLUDES} && cp "${FILECOIN_FFI_PATH}/filecoin.h" ${FILECOIN_FFI_INCLUDES}
        WORKING_DIRECTORY ${FILECOIN_FFI_PATH}
)

add_dependencies(filecoin_ffi_fix_include
        filecoin_ffi_build
        )

add_library(filecoin_ffi
        STATIC IMPORTED GLOBAL
        )

set(ENV{PKG_CONFIG_PATH}  "${PKG_CONFIG_PATH}:${FILECOIN_FFI_PATH}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(PKG_FILECOIN REQUIRED filecoin)

target_link_libraries(filecoin_ffi INTERFACE ${PKG_FILECOIN_LIBRARIES})
target_include_directories(filecoin_ffi PUBLIC ${PKG_FILECOIN_INCLUDE_DIRS})
target_compile_options(filecoin_ffi PUBLIC ${PKG_FILECOIN_CFLAGS_OTHER})

set_target_properties(filecoin_ffi PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${FILECOIN_FFI_PATH}/include
        IMPORTED_LOCATION ${FILECOIN_FFI_LIB}
        )

add_dependencies(filecoin_ffi
        filecoin_ffi_fix_include
        )
