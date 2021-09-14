#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(miner_actor_state_test
    miner_actor_state_v0_test.cpp
    miner_actor_state_v2_test.cpp
    miner_actor_state_v3_test.cpp
    )
target_link_libraries(miner_actor_state_test
    ipfs_datastore_in_memory
    miner_types
    miner_actor_state
    )
