#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(market_actor_v3
    market_actor_utils.cpp
    )
target_link_libraries(market_actor_v3
    market_actor_state
    market_actor_v2
    actor
    array
    balance_table
    cid_key
    piece
    )
