#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(miner_actor_v0
    miner_actor.cpp
    miner_actor_utils.cpp
    )
target_link_libraries(miner_actor_v0
    miner_actor_state
    actor
    address
    array
    cbor
    exit_code
    map
    rle_bitset
    sector
    uvarint_key
    )
