#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_array_test
    array_test.cpp
    )
target_link_libraries(filecoin_array_test
    filecoin_amt
    filecoin_array
    filecoin_ipfs_datastore_in_memory
    )

addtest(filecoin_balance_table_hamt_test
    balance_table_hamt_test.cpp
    )
target_link_libraries(filecoin_balance_table_hamt_test
    filecoin_balance_table_hamt
    filecoin_ipfs_datastore_in_memory
    )

addtest(filecoin_multimap_test
    multimap_test.cpp
    )
target_link_libraries(filecoin_multimap_test
    filecoin_hamt
    filecoin_multimap
    filecoin_ipfs_datastore_in_memory
    )
