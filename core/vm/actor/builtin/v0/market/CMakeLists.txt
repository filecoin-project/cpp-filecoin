#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(market_actor
    actor.cpp
    )
target_link_libraries(market_actor
    actor
    array
    balance_table
    piece
    )
