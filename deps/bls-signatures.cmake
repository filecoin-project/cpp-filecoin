#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

if(EXISTS "$ENV{HOME}/.cargo/bin/cargo")
  message(STATUS "Rust cargo is present")
else()
  message(STATUS "No Rust cargo found")
  message(WARNING
          "Rust is required for building BLS dependencies.\n"
          "You can install rust-lang via the following command:\n"
          "curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh"
  )
endif()

set(BUILD_TYPE "release")

set(BLS_BUILD_PATH
    "${CMAKE_BINARY_DIR}/deps/bls-signatures"
    )

set(BLS_LIBRARY
    ${BLS_BUILD_PATH}/${BUILD_TYPE}/${CMAKE_SHARED_LIBRARY_PREFIX}bls_signatures_ffi${CMAKE_STATIC_LIBRARY_SUFFIX}
    )

add_custom_target(
    cargo_build
    ALL
    COMMAND $ENV{HOME}/.cargo/bin/cargo build -p bls-signatures-ffi --target-dir ${BLS_BUILD_PATH} --${BUILD_TYPE}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/deps/bls-signatures"
    BYPRODUCTS ${BLS_LIBRARY}
  )

add_library(bls_signatures_ffi
    STATIC IMPORTED GLOBAL
    )

add_dependencies(bls_signatures_ffi
    cargo_build
    )

file(MAKE_DIRECTORY ${BLS_BUILD_PATH}/${BUILD_TYPE})

set_target_properties(bls_signatures_ffi PROPERTIES
    IMPORTED_LOCATION ${BLS_LIBRARY}
    )
