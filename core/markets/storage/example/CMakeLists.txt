#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

set(EXAMPLE_RESOURCES_PATH "${PROJECT_SOURCE_DIR}/core/markets/storage/example/")
configure_file(
    "${PROJECT_SOURCE_DIR}/core/markets/storage/example/resources.hpp.in"
    "${PROJECT_SOURCE_DIR}/core/markets/storage/example/resources.hpp"
)

add_executable(example_provider_main
    provider_main.cpp
    )
target_link_libraries(example_provider_main
    p2p::p2p
    storage_market_provider
    keystore
    secp256k1_provider
    bls_provider
    p2p::p2p_literals
    in_memory_storage
    ipfs_datastore_in_memory
    pieceio
    )

add_executable(example_client_main
    client_main.cpp
    )
target_link_libraries(example_client_main
    p2p::p2p
    storage_market_client
    keystore
    secp256k1_provider
    bls_provider
    p2p::p2p_literals
    in_memory_storage
    ipfs_datastore_in_memory
    pieceio
    )

add_executable(storage_market_example
    storage_market_example.cpp
    )
target_link_libraries(storage_market_example
    ipfs_datastore_in_memory
    pieceio
    todo_error
    )
target_include_directories(storage_market_example PUBLIC
    "${TEST_RESOURCES_PATH}"
    )
