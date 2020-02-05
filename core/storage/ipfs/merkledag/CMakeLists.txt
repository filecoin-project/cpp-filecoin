find_package(Protobuf REQUIRED)
set(PB_SCHEME "merkledag.proto")
set(PB_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

execute_process(COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --cpp_out=${PB_BUILD_DIR} ${PB_SCHEME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/impl/protobuf
    RESULT_VARIABLE CMD_OUTPUT
    )

add_library(ipfs_merkledag_service_protobuf
    ${PB_BUILD_DIR}/merkledag.pb.h
    ${PB_BUILD_DIR}/merkledag.pb.cc
    )
target_include_directories(ipfs_merkledag_service_protobuf PUBLIC ${PB_BUILD_DIR})
target_link_libraries(ipfs_merkledag_service_protobuf
    protobuf::libprotobuf
    )
disable_clang_tidy(ipfs_merkledag_service_protobuf)

add_library(ipfs_merkledag_service
    impl/link_impl.cpp
    impl/node_impl.cpp
    impl/merkledag_service_impl.cpp
    impl/pb_node_encoder.cpp
    impl/pb_node_decoder.cpp
    impl/leaf_impl.cpp
    )
target_link_libraries(ipfs_merkledag_service
    cid
    Boost::boost
    ipfs_blockservice
    ipfs_datastore_in_memory
    ipfs_merkledag_service_protobuf
    )
