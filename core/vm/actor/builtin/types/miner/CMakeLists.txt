#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(miner_types
    deadline_assignment_heap.cpp
    deadline_info.cpp
    deadline_sector_map.cpp
    partition_sector_map.cpp
    deadline.cpp
    deadlines.cpp
    expiration.cpp
    partition.cpp
    quantize.cpp
    sectors.cpp
    termination.cpp
    vesting.cpp
    v0/expiration.cpp
    v0/partition.cpp
    v0/deadline.cpp
    v2/expiration.cpp
    v2/partition.cpp
    v2/deadline.cpp
    v3/partition.cpp
    v3/deadline.cpp
    bind_types.cpp
    )
target_link_libraries(miner_types
    actor
    address
    const
    outcome
    type_manager
    )
