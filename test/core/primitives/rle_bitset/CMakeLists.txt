#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(rle_bitset_test
    rle_bitset_test.cpp
    )
target_link_libraries(rle_bitset_test
    rle_bitset
    hexutil
    )
