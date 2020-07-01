#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(market_network
    impl/libp2p_market_network.cpp
    )
target_link_libraries(market_network
    cbor_stream
    p2p::p2p
    logger
    )
