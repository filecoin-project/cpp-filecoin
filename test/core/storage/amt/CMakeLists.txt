#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_amt_test
    amt_test.cpp
    )
target_link_libraries(filecoin_amt_test
    filecoin_amt
    filecoin_hexutil
    filecoin_ipfs_datastore_in_memory
    )
