#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#


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
