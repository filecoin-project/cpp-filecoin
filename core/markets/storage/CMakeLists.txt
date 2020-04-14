#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(client)
add_subdirectory(provider)

add_library(fuhon_stored_ask stored_ask.cpp)
target_link_libraries(fuhon_stored_ask
    p2p::p2p
    )

add_library(libp2p_storage_market_network
    network/libp2p_storage_market_network.cpp
    network/libp2p_ask_stream.cpp
    network/libp2p_deal_stream.cpp
    )
target_link_libraries(libp2p_storage_market_network
    p2p::p2p
    logger
    )
