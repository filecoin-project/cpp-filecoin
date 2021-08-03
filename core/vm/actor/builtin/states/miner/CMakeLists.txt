#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(miner_actor_state
    v0/miner_actor_state.cpp
    v2/miner_actor_state.cpp
    v3/miner_actor_state.cpp
    bind_states.cpp
    )
target_link_libraries(miner_actor_state
    cbor
    toolchain
    miner_types
    )
