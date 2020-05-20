#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

set(EXAMPLE_RESOURCES_PATH "${PROJECT_SOURCE_DIR}/core/markets/storage/example/")
configure_file(
    "${PROJECT_SOURCE_DIR}/core/markets/storage/example/resources.hpp.in"
    "${PROJECT_SOURCE_DIR}/core/markets/storage/example/resources.hpp"
)

add_library(storage_market_provider_example
    provider_example.cpp
    )
target_link_libraries(storage_market_provider_example
    storage_market_provider
    keystore
    )

add_library(storage_market_client_example
    client_example.cpp
    )
target_link_libraries(storage_market_client_example
    storage_market_client
    keystore
    )

add_executable(storage_market_example
    storage_market_example.cpp
    )
target_link_libraries(storage_market_example
    in_memory_storage
    ipfs_datastore_in_memory
    pieceio
    todo_error
    secp256k1_provider
    bls_provider
    pieceio
    storage_market_provider_example
    storage_market_client_example
    p2p::p2p_literals
    )
target_include_directories(storage_market_example PUBLIC
    "${TEST_RESOURCES_PATH}"
    )
