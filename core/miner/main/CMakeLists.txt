#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(miner_main
    main.cpp
    )
target_link_libraries(miner_main
    Boost::program_options
    filecoin_config
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
    remote_worker
    storage_miner_api

    chain_events
    data_transfer
    ipfs_datastore_leveldb
    pieceio
    retrieval_market_provider
    storage_market_provider

    ipfs_datastore_in_memory

    miner_actor_state
    )
