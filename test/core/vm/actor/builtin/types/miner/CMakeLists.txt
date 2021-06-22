#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(miner_types_test
    quantize_test.cpp
    bitfield_queue_test.cpp
    sectors_test.cpp
    termination_test.cpp
    )
target_link_libraries(miner_types_test
    ipfs_datastore_in_memory
    miner_types
    )
