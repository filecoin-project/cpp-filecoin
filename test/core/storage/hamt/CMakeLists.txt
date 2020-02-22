#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_hamt_test
    hamt_test.cpp
    )
target_link_libraries(filecoin_hamt_test
    filecoin_hamt
    filecoin_hexutil
    filecoin_ipfs_datastore_in_memory
    )
