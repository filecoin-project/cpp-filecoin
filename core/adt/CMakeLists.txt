#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_custom_target(filecoin_adt
    DEPENDS
    filecoin_array
    filecoin_multimap
    )

filecoin_add_library(filecoin_array
    impl/array.cpp
    )
target_link_libraries(filecoin_array
    filecoin_amt
    )

filecoin_add_library(filecoin_balance_table_hamt
    impl/balance_table_hamt.cpp
    )
target_link_libraries(filecoin_balance_table_hamt
    filecoin_address
    filecoin_hamt
    )

filecoin_add_library(filecoin_multimap
    impl/multimap.cpp
    )
target_link_libraries(filecoin_multimap
    filecoin_array
    filecoin_hamt
    )
