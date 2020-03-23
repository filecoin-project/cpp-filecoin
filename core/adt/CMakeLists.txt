#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_custom_target(adt
    DEPENDS
    address_key
    array
    balance_table
    map
    multimap
    uvarint_key
    )

add_library(array INTERFACE)
target_link_libraries(array INTERFACE
    amt
    )

add_library(map INTERFACE)
target_link_libraries(map INTERFACE
    hamt
    )

add_library(balance_table_hamt
    impl/balance_table_hamt.cpp
    )
target_link_libraries(balance_table_hamt
    address_key
    hamt
    )

add_library(multimap
    impl/multimap.cpp
    )
target_link_libraries(multimap
    amt
    hamt
    )

add_library(uvarint_key
    uvarint_key.cpp
    )
target_link_libraries(uvarint_key
    p2p::p2p_uvarint
    )

add_library(address_key
    address_key.cpp
    )
target_link_libraries(address_key
    address
    )

add_library(balance_table
    balance_table.cpp
    )
target_link_libraries(balance_table
    address_key
    map
    )
