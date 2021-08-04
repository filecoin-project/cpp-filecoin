#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(actor_state
    impl/bind_states.cpp
    )
target_link_libraries(actor_state
    actor
    outcome
    storage_power_types
    )

add_subdirectory(account)
add_subdirectory(cron)
add_subdirectory(init)
add_subdirectory(market)
add_subdirectory(miner)
add_subdirectory(multisig)
add_subdirectory(payment_channel)
add_subdirectory(reward)
add_subdirectory(storage_power)
add_subdirectory(system)
add_subdirectory(verified_registry)
