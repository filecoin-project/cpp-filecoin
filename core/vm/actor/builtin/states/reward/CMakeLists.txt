#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(reward_actor_state
    reward_actor_state.cpp
    v0/reward_actor_state.cpp
    v2/reward_actor_state.cpp
    bind_states.cpp
    )
target_link_libraries(reward_actor_state
    cbor
    reward_types
    )
