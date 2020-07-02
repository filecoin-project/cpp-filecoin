#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(chain_events)
add_subdirectory(provider)

addtest(storage_market_test
    storage_market_types_encoding_test.cpp
    storage_market_ask_test.cpp
    storage_market_deal_test.cpp
    )
target_link_libraries(storage_market_test
    address
    bls_provider
    cbor
    secp256k1_provider
    piece
    pieceio
    in_memory_storage
    ipfs_datastore_in_memory
    storage_market_client
    storage_market_provider
    keystore
    )
