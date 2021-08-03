#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(market_actor_state
    market_actor_state.cpp
    bind_states.cpp
    )
target_link_libraries(market_actor_state
    cbor
    toolchain
    )
