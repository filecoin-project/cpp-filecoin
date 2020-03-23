#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(balance_table_hamt_test
    balance_table_hamt_test.cpp
    )
target_link_libraries(balance_table_hamt_test
    balance_table_hamt
    ipfs_datastore_in_memory
    )

addtest(multimap_test
    multimap_test.cpp
    )
target_link_libraries(multimap_test
    hamt
    multimap
    ipfs_datastore_in_memory
    )
