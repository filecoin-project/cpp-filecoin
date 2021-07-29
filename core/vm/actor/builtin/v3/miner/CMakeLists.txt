#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(miner_actor_v3
    miner_actor.cpp
    miner_actor_state.cpp
    miner_actor_utils.cpp
    types/partition.cpp
    )
target_link_libraries(miner_actor_v3
    miner_actor_v2
    miner_types
    )
