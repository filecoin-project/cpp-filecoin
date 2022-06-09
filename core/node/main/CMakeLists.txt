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
    chain_events
    cids_index
    clock
    compacter
    drand_beacon
    drand_http
    graphsync
    in_memory_storage
    ipfs_datastore_in_memory
    ipfs_datastore_leveldb
    interpreter
    keystore
    mpool
    node_version
    paych_maker
    paych_vouchers
    p2p::p2p_basic_host
    p2p::p2p_default_network
    p2p::p2p_peer_repository
    p2p::p2p_inmem_address_repository
    p2p::p2p_inmem_key_repository
    p2p::p2p_inmem_protocol_repository
    p2p::p2p_random_generator
    pieceio
    retrieval_market_client
    rpc
    runtime
    secp256k1_provider
    storage_market_client
    storage_market_import_manager
    sync
    weight_calculator
    miner_types
    wallet
    )

add_executable(fuhon-node
    main.cpp
    )
target_link_libraries(fuhon-node
    node_builder
    )
set_target_properties(fuhon-node PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
