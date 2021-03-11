#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(market_actor_v2
    market_actor_state.cpp
    )
target_link_libraries(market_actor_v2
    actor
    array
    balance_table
    cid_key
    piece
    )
