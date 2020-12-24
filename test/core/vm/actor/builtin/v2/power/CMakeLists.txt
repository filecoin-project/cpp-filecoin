#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(storage_power_actor_v2_test
    storage_power_actor_v2_test.cpp
    )
target_link_libraries(storage_power_actor_v2_test
    ipfs_datastore_in_memory
    storage_power_actor_v2
    state_tree
    )
