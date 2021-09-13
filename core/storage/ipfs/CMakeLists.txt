#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(ipfs_datastore_error
    impl/ipfs_datastore_error.cpp
    )
target_link_libraries(ipfs_datastore_error
    outcome
    )

add_library(ipfs_datastore_in_memory
    impl/in_memory_datastore.cpp
    )
target_link_libraries(ipfs_datastore_in_memory
    buffer
    cbor
    cid
    ipfs_datastore_error
    )

add_library(ipfs_datastore_leveldb
    impl/datastore_leveldb.cpp
    )
target_link_libraries(ipfs_datastore_leveldb
    buffer
    cbor
    cid
    ipfs_datastore_error
    leveldb
    )

add_subdirectory(graphsync)
add_subdirectory(api_ipfs_datastore)
