#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_power_actor
    storage_power_actor_state.cpp
    )
target_link_libraries(storage_power_actor
    actor
    outcome
    power_table
    hamt
    multimap
    )
