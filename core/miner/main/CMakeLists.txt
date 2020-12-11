#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(miner_main
    main.cpp
    )
target_link_libraries(miner_main
    Boost::program_options
    clock
    leveldb
    manager
    miner
    mining
    p2p::p2p
    rpc
    store

    ipfs_datastore_in_memory
    )
