#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_rle_bitset_test
    rle_bitset_test.cpp
    )
target_link_libraries(filecoin_rle_bitset_test
    filecoin_rle_bitset
    filecoin_hexutil
    )
