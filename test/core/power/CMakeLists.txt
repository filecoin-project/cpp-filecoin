# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_power_table_test
    power_table_test.cpp
    )
target_link_libraries(filecoin_power_table_test
    filecoin_power_table
    filecoin_base_fs_test
    )

addtest(filecoin_power_table_hamt_test
    power_table_hamt_test.cpp
    )
target_link_libraries(filecoin_power_table_hamt_test
    filecoin_power_table_hamt
    filecoin_ipfs_datastore_in_memory
    )
