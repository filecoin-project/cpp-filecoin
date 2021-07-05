#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(actor_state
    impl/state_provider.cpp
    impl/state_manager_impl.cpp
    impl/market_actor_state.cpp
    impl/multisig_actor_state.cpp
    impl/power_actor_state.cpp
    )
target_link_libraries(actor_state
    actor
    actors_toolchain
    outcome
    )
