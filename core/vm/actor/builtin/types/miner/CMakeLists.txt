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
    )
target_link_libraries(miner_types
    actor
    address
    const
    outcome
    )
