#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(miner_types_test
    quantize_test.cpp
    bitfield_queue_test.cpp
    deadline_sector_map_test.cpp
    sectors_test.cpp
    partition_sector_map_test.cpp
    termination_test.cpp
    expiration_set_test.cpp
    expiration_queue_v0_test.cpp
    expiration_queue_v2_test.cpp
    expiration_queue_v3_test.cpp
    )
target_link_libraries(miner_types_test
    ipfs_datastore_in_memory
    miner_types
    miner_actor_v0
    miner_actor_v2
    miner_actor_v3
    )
