#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(sync
    common.cpp
    peer_manager.cpp
    hello.cpp
    pubsub_gate.cpp
    object_loader.cpp
    block_loader.cpp
    tipset_loader.cpp
    blocksync_common.cpp
    blocksync_client.cpp
    #sync_job.cpp
    tipset_graph.cpp
    branches.cpp
    index_db.cpp
    index_db_backend.cpp
    )

target_link_libraries(sync
    cbor_stream
    p2p::libp2p_sqlite
    p2p::p2p_gossip
    p2p::p2p_identify)
