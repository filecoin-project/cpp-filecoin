#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(node_builder
    config.cpp
    builder.cpp
    )

target_link_libraries(node_builder
    filecoin_config
    Boost::program_options
    Boost::random
    block_validator
    car
    cids_index
    clock
    drand_beacon
    drand_http
    graphsync
    in_memory_storage
    ipfs_datastore_in_memory
    ipfs_datastore_leveldb
    interpreter
    mpool
    p2p::p2p_basic_host
    p2p::p2p_default_network
    p2p::p2p_peer_repository
    p2p::p2p_inmem_address_repository
    p2p::p2p_inmem_key_repository
    p2p::p2p_inmem_protocol_repository
    p2p::asio_scheduler
    p2p::p2p_random_generator
    pieceio
    rpc
    runtime
    secp256k1_provider
    storage_market_client
    sync
    weight_calculator
    )

add_executable(node
    main.cpp
    )

target_link_libraries(node
    node_builder
    )