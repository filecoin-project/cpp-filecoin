#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(miner_types_test
    quantize_test.cpp
    bitfield_queue_test.cpp
    )
target_link_libraries(miner_types_test
    miner_types
    )
