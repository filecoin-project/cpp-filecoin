#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#
filecoin_add_library(filecoin_ipfs_datastore_in_memory
    impl/in_memory_datastore.cpp
    impl/ipfs_datastore_error.cpp
    )
target_link_libraries(filecoin_ipfs_datastore_in_memory
    filecoin_buffer
    filecoin_cbor
    filecoin_cid
    )

filecoin_add_library(filecoin_ipfs_datastore_leveldb
    impl/datastore_leveldb.cpp
    impl/ipfs_datastore_error.cpp
    )
target_link_libraries(filecoin_ipfs_datastore_leveldb
    filecoin_buffer
    filecoin_cbor
    filecoin_cid
    filecoin_leveldb
    )

filecoin_add_library(filecoin_ipfs_blockservice
    impl/ipfs_block_service.cpp
    )
target_link_libraries(filecoin_ipfs_blockservice
    filecoin_buffer
    )

add_subdirectory(merkledag)
