#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(sync
    peer_manager.cpp
    hello.cpp
    pubsub_gate.cpp
    object_loader.cpp
    )

target_link_libraries(sync
    p2p::p2p_gossip
    p2p::p2p_identify)
