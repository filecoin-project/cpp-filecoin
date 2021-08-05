#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(reward_actor_v2
    reward_actor.cpp
    )
target_link_libraries(reward_actor_v2
    reward_actor_state
    reward_actor_v0
    )
