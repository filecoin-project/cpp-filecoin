#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(miner_actor
    miner_actor.cpp
    )
target_link_libraries(miner_actor
    address
    cbor
    p2p::p2p_uvarint
    rle_bitset
    )
