# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

filecoin_add_library(filecoin_power_table
    impl/power_table_impl.cpp
    impl/power_table_error.cpp
    )
target_link_libraries(filecoin_power_table
    filecoin_address
    filecoin_outcome
    )

filecoin_add_library(filecoin_power_table_hamt
    impl/power_table_hamt.cpp
    impl/power_table_error.cpp
    )
target_link_libraries(filecoin_power_table_hamt
    filecoin_address
    filecoin_outcome
    filecoin_hamt
    )
