#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_market_provider
    impl/provider_impl.cpp
    impl/storage_provider_error.cpp
    )
target_link_libraries(storage_market_provider
    Boost::boost
    api
    ask_protocol_cbor
    fuhon_stored_ask
    outcome
    piece
    piece_storage
    sectorblocks
    )

add_library(fuhon_stored_ask
    impl/stored_ask.cpp
    )
target_link_libraries(fuhon_stored_ask
    p2p::p2p
    piece
    tipset
    )
