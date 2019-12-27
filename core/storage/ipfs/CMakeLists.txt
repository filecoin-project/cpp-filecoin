#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#
add_library(ipfs_datastore_in_memory
    impl/in_memory_datastore.cpp
    impl/ipfs_datastore_error.cpp
    )
target_link_libraries(ipfs_datastore_in_memory
    buffer
    cbor
    cid
    )

add_library(ipfs_datastore_leveldb
    impl/datastore_leveldb.cpp
    impl/ipfs_datastore_error.cpp
    )
target_link_libraries(ipfs_datastore_leveldb
    buffer
    cbor
    cid
    leveldb
    )
