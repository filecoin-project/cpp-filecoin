#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_market_client
    client_impl.cpp
    )
target_link_libraries(storage_market_client
    address
    blob
    discovery
    fuhon_fsm
    outcome
    logger
    libp2p_storage_market_network
    piece
    signature
    market_actor
    message
    tipset
    )
