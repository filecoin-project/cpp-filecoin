#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(builtin)
add_subdirectory(cgo)

add_library(actor
    actor.cpp
    impl/invoker_impl.cpp
    )
target_link_libraries(actor
    address
    buffer
    cid
    exit_code
    account_actor_v0
    account_actor_v2
    account_actor_v3
    init_actor_v0
    market_actor_v0
    miner_actor_v0
    multisig_actor_v0
    multisig_actor_v2
    multisig_actor_v3
    storage_power_actor_v0
    storage_power_actor_v2
    cron_actor_v0
    cron_actor_v2
    reward_actor_v0
    reward_actor_v2
    payment_channel_actor_v0
    payment_channel_actor_v2
    payment_channel_actor_v3
    system_actor_v0
    system_actor_v2
    system_actor_v3
    cgo_actors
    )
