#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(actor_state
    impl/multisig_actor_state.cpp
    impl/power_actor_state.cpp
    impl/reward_actor_state.cpp
    impl/bind_states.cpp
    )
target_link_libraries(actor_state
    actor
    outcome
    miner_types
    storage_power_types
    )

add_subdirectory(account)
add_subdirectory(cron)
add_subdirectory(init)
add_subdirectory(market)
add_subdirectory(miner)
