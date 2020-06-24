#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(node_builder
    config.cpp
    builder.cpp)

target_link_libraries(node_builder
    sync
    p2p::p2p_basic_host
    p2p::p2p_default_network
    p2p::p2p_peer_repository
    p2p::p2p_inmem_address_repository
    p2p::p2p_inmem_key_repository
    p2p::p2p_inmem_protocol_repository
    p2p::asio_scheduler
    graphsync
    clock
    car
    chain_store
    chain_data_store
    block_validator
    interpreter
    rpc
    runtime
    ipfs_datastore_in_memory
    weight_calculator
    secp256k1_provider
    Boost::program_options
    mpool
    msg_waiter
    )

add_executable(node_main
    main.cpp
    )
target_link_libraries(node_main
    node_builder
    sync
    )

add_subdirectory(integration)
