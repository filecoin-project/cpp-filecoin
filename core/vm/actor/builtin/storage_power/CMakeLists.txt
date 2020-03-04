#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_power_actor
    storage_power_actor_state.cpp
    storage_power_actor_export.cpp
    policy.cpp
    )
target_link_libraries(storage_power_actor
    actor
    block
    builtin_shared
    chain_epoch_codec
    balance_table_hamt
    hamt
    multimap
    )
