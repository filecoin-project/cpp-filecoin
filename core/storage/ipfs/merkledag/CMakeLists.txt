#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_ipfs_merkledag_service
    impl/merkledag_service_impl.cpp
    impl/leaf_impl.cpp
    )
target_link_libraries(filecoin_ipfs_merkledag_service
    Boost::boost
    filecoin_ipld_node
    filecoin_ipfs_blockservice
    filecoin_ipfs_datastore_in_memory
    )
