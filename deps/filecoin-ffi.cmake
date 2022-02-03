#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

set(FILECOIN_FFI_PATH "${PROJECT_SOURCE_DIR}/deps/our-filecoin-ffi")

set(FILECOIN_FFI_INCLUDES
        "${FILECOIN_FFI_PATH}/include/filecoin-ffi"
        )

set(FILECOIN_FFI_LIB
        "${FILECOIN_FFI_PATH}/lib/libfilcrypto.a"
        )

set(FILECOIN_FFI_PKG
        "${FILECOIN_FFI_PATH}/lib/pkgconfig"
        )

set(ENV{PKG_CONFIG_PATH}  "${PKG_CONFIG_PATH}:${FILECOIN_FFI_PKG}")

# uncomment for build from source
#set(ENV{FFI_BUILD_FROM_SOURCE}  "1")

find_package(PkgConfig REQUIRED)
pkg_check_modules(PKG_FILECOIN IMPORTED_TARGET filcrypto)

if (NOT PKG_FILECOIN_FOUND)
    message("Installing filecoin-ffi")
    execute_process(COMMAND ./install-filcrypto.sh
            WORKING_DIRECTORY ${FILECOIN_FFI_PATH})

    pkg_check_modules(PKG_FILECOIN REQUIRED IMPORTED_TARGET filcrypto)
endif (NOT PKG_FILECOIN_FOUND)



add_custom_target(
        filecoin_ffi_build
        COMMAND ./install-filcrypto.sh
        WORKING_DIRECTORY ${FILECOIN_FFI_PATH}
)

add_library(filecoin_ffi
        STATIC IMPORTED GLOBAL
        )

set_target_properties(filecoin_ffi PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${FILECOIN_FFI_PATH}/include
        IMPORTED_LOCATION ${FILECOIN_FFI_LIB}
        )

target_link_libraries(filecoin_ffi INTERFACE PkgConfig::PKG_FILECOIN)

add_dependencies(filecoin_ffi
        filecoin_ffi_build
        )
