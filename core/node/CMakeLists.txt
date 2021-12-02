#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(main)

add_library(sync
    identify.cpp
    say_hello.cpp
    receive_hello.cpp
    pubsub_gate.cpp
    blocksync_request.cpp
    blocksync_server.cpp
    graphsync_server.cpp
    sync_job.cpp
    head_constructor.cpp
    chain_store_impl.cpp
    pubsub_workaround.cpp
    peer_discovery.cpp
    )

target_link_libraries(sync
    cbor_stream
    interpreter
    p2p::p2p_gossip
    p2p::p2p_identify
    p2p::p2p_kademlia
    )

add_library(node_version INTERFACE
    )
target_link_libraries(node_version INTERFACE
    git_commit_version
    )
