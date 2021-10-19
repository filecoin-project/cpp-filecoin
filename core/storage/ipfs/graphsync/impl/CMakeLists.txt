#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(network/marshalling/protobuf)

add_library(graphsync
    common.cpp
    graphsync_impl.cpp
    local_requests.cpp
    network/network.cpp
    network/peer_context.cpp
    network/length_delimited_message_reader.cpp
    network/message_reader.cpp
    network/message_queue.cpp
    network/outbound_endpoint.cpp
    network/marshalling/serialize.cpp
    network/marshalling/message_parser.cpp
    network/marshalling/message_builder.cpp
    network/marshalling/request_builder.cpp
    network/marshalling/response_builder.cpp
    )

target_include_directories(graphsync PRIVATE "${CMAKE_BINARY_DIR}/generated/storage/ipfs/graphsync/impl/network/marshalling")

target_link_libraries(graphsync
    Boost::boost
    p2p::asio_scheduler
    p2p::subscription
    bytes
    cid
    cbor
    cbor_types
    filecoin_hasher
    logger
    graphsync_proto
    )
