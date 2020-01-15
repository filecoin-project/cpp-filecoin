#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(block_test
    block_test.cpp
    )
target_link_libraries(block_test
    address
    cbor
    hexutil
    tickets
    )
