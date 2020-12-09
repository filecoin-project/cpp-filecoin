#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(sync
    peers.cpp
    identify.cpp
    say_hello.cpp
    receive_hello.cpp
    pubsub_gate.cpp
    tipset_loader.cpp
    blocksync_client.cpp
    blocksync_server.cpp
    subchain_loader.cpp
    syncer.cpp
    interpret_job.cpp
    chain_db.cpp
    branches.cpp
    index_db.cpp
    index_db_backend.cpp
    head_constructor.cpp
    chain_store_impl.cpp
    pubsub_workaround.cpp
    peer_discovery.cpp
    )

target_link_libraries(sync
    cbor_stream
    p2p::libp2p_sqlite
    p2p::p2p_gossip
    p2p::p2p_identify
    p2p::p2p_kademlia)

add_subdirectory(main)
