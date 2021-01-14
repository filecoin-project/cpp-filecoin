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
    proof_param_provider
    p2p::p2p
    rpc
    store
    fetch_handler

    ipfs_datastore_in_memory
    )
