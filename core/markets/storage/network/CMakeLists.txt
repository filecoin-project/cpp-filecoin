#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(libp2p_storage_market_network
    libp2p_storage_market_network.cpp
    )
target_link_libraries(libp2p_storage_market_network
    cbor_stream
    p2p::p2p
    logger
    )
