#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(market_actor_v0
    market_actor_utils.cpp
    actor.cpp
    )
target_link_libraries(market_actor_v0
    actor
    array
    balance_table
    piece
    )
