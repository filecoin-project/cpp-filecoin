#
#Copyright Soramitsu Co., Ltd.All Rights Reserved.
#SPDX - License - Identifier : Apache - 2.0
#

find_package(Protobuf REQUIRED)
set(PB_SCHEME "ipld_node.proto")
set(PB_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

execute_process(COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --cpp_out=${PB_BUILD_DIR} ${PB_SCHEME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/impl/protobuf
    RESULT_VARIABLE CMD_OUTPUT
    )

add_library(ipld_node_protobuf
    ${PB_BUILD_DIR}/ipld_node.pb.h
    ${PB_BUILD_DIR}/ipld_node.pb.cc
    )
target_include_directories(ipld_node_protobuf PUBLIC ${PB_BUILD_DIR})
target_link_libraries(ipld_node_protobuf
    protobuf::libprotobuf
    )
disable_clang_tidy(ipld_node_protobuf)

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
