#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(reward_actor_v0
    reward_actor.cpp
    reward_actor_state.cpp
    )
target_link_libraries(reward_actor_v0
    Boost::boost
    actor
    address
    exit_code
    reward_types
    outcome
    )
