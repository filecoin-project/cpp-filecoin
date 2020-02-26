#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(ipfs_merkledag_service
    impl/merkledag_service_impl.cpp
    impl/leaf_impl.cpp
    )
target_link_libraries(ipfs_merkledag_service
    Boost::boost
    ipld_node
    ipfs_blockservice
    ipfs_datastore_in_memory
    )
