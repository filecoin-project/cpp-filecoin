#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(ipfs_leveldb
    impl/datastore_leveldb.cpp
    )
target_link_libraries(ipfs_leveldb
    p2p::p2p_cid
    buffer
    )
