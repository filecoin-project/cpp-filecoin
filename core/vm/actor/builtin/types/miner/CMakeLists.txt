#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(miner_types
    deadline_info.cpp
    quantize.cpp
    bitfield_queue.cpp
    expiration.cpp
    partition.cpp
    termination.cpp
    )
target_link_libraries(miner_types
    actor
    address
    const
    outcome
    )
