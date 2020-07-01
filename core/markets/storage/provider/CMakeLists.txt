#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_market_provider
    impl/provider_impl.cpp
    impl/storage_provider_error.cpp
    impl/provider_state_store.cpp
    impl/provider_data_transfer_request_validator.cpp
    )
target_link_libraries(storage_market_provider
    Boost::boost
    api
    fuhon_fsm
    fuhon_stored_ask
    outcome
    piece
    market_actor
    todo_error
    piece_storage
    )

add_library(fuhon_stored_ask
    impl/stored_ask.cpp
    )
target_link_libraries(fuhon_stored_ask
    p2p::p2p
    piece
    tipset
    )
