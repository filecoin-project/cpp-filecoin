#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(protobuf)

add_library(graphsync
    errors.cpp
    session.cpp
    graphsync_impl.cpp
    merkledag_bridge_impl.cpp
    block_subscription.cpp
    local_requests.cpp
    marshalling/serialize.cpp
    marshalling/message_parser.cpp
    marshalling/message_builder.cpp
    marshalling/request_builder.cpp
    marshalling/response_builder.cpp
    network/server.cpp
    network/connector.cpp
    network/length_delimited_message_reader.cpp
    network/graphsync_message_reader.cpp
    network/out_msg_queue.cpp
    )

target_include_directories(graphsync PRIVATE "${CMAKE_BINARY_DIR}/generated/storage/ipfs/graphsync/impl")

target_link_libraries(graphsync
    Boost::boost
    p2p_multiaddress
    p2p_varint_reader
    p2p_peer_id
    p2p_cid
    graphsync_proto
    )
