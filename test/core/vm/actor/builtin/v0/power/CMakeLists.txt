#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(storage_power_actor_test_v0
    storage_power_actor_v0_test.cpp
    )
target_link_libraries(storage_power_actor_test_v0
    ipfs_datastore_in_memory
    storage_power_actor_v0
    )
