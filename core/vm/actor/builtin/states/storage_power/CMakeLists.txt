#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_power_actor_state
    storage_power_actor_state.cpp
    v0/storage_power_actor_state.cpp
    v2/storage_power_actor_state.cpp
    bind_states.cpp
    )
target_link_libraries(storage_power_actor_state
    cbor
    storage_power_types
    )
