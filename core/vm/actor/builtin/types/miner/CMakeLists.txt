#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(miner_types
    bitfield_queue.cpp
    deadline_info.cpp
    deadline_sector_map.cpp
    expiration.cpp
    partition_sector_map.cpp
    partition.cpp
    quantize.cpp
    sectors.cpp
    termination.cpp
    v0/expiration.cpp
    v0/partition.cpp
    v2/expiration.cpp
    v2/partition.cpp
    v3/partition.cpp
    bind_types.cpp
    v0/monies.cpp
    v2/monies.cpp
    v3/monies.cpp
    )
target_link_libraries(miner_types
    actor
    address
    const
    outcome
    type_manager
    )
