# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(power_table_test
    power_table_test.cpp
    )
target_link_libraries(power_table_test
    power_table
    base_fs_test
    )

addtest(power_table_hamt_test
    power_table_hamt_test.cpp
    )
target_link_libraries(power_table_hamt_test
    power_table_hamt
    ipfs_datastore_in_memory
    )
