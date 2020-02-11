#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(amt_test
    amt_test.cpp
    )
target_link_libraries(amt_test
    amt
    hexutil
    ipfs_datastore_in_memory
    )
