# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(power_table
        impl/power_table_impl.cpp
        impl/power_table_error.cpp
        )
target_link_libraries(power_table
        outcome
        address
        )
