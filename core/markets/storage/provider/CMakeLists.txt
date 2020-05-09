#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_market_provider
    provider.cpp
    storage_provider_error.cpp
    )
target_link_libraries(storage_market_provider
    Boost::boost
    api
    fuhon_fsm
    fuhon_stored_ask
    outcome
    piece
    libp2p_storage_market_network
    todo_error
    )

add_library(fuhon_stored_ask
    stored_ask.cpp
    )
target_link_libraries(fuhon_stored_ask
    p2p::p2p
    piece
    tipset
    )
