#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_storage_power_actor
    storage_power_actor_state.cpp
    storage_power_actor_export.cpp
    policy.cpp
    )
target_link_libraries(filecoin_storage_power_actor
    filecoin_actor
    filecoin_block
    filecoin_builtin_shared
    filecoin_chain_epoch_codec
    filecoin_balance_table_hamt
    filecoin_hamt
    filecoin_multimap
    )
