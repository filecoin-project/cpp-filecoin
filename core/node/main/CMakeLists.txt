#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(node_builder
    config.cpp
    builder.cpp
    )

target_link_libraries(node_builder
    sync
    p2p::p2p_basic_host
    p2p::p2p_default_network
    p2p::p2p_peer_repository
    p2p::p2p_inmem_address_repository
    p2p::p2p_inmem_key_repository
    p2p::p2p_inmem_protocol_repository
    p2p::asio_scheduler
    p2p::p2p_random_generator
    graphsync
    cids_index
    clock
    car
    block_validator
    interpreter
    runtime
    ipfs_datastore_leveldb
    ipfs_datastore_in_memory
    in_memory_storage
    weight_calculator
    secp256k1_provider
    rpc
    mpool
    drand_http
    drand_beacon
    Boost::program_options
    Boost::random
    )

add_executable(node
    main.cpp
    )

target_link_libraries(node
    node_builder
    )
