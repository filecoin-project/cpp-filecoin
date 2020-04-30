#
#Copyright Soramitsu Co., Ltd.All Rights Reserved.
#SPDX - License - Identifier : Apache - 2.0
#

add_subdirectory(impl/protobuf)

add_library(ipld_block INTERFACE)
target_link_libraries(ipld_block INTERFACE
    cid
    filecoin_hasher
    )

add_library(ipld_link
    impl/ipld_link_impl.cpp
    )
target_link_libraries(ipld_link
    cid
    )

add_library(ipld_node
    impl/ipld_node_impl.cpp
    impl/ipld_node_encoder_pb.cpp
    impl/ipld_node_decoder_pb.cpp
    )
target_link_libraries(ipld_node
    ipld_node_protobuf
    ipld_link
    ipld_block
    Boost::boost
    cid
    )
