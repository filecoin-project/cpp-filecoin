# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(power_table
    impl/power_table_impl.cpp
    impl/power_table_error.cpp
    )
target_link_libraries(power_table
    address
    outcome
    )

add_library(power_table_hamt
    impl/power_table_hamt.cpp
    impl/power_table_error.cpp
    )
target_link_libraries(power_table_hamt
    address_key
    outcome
    hamt
    )
