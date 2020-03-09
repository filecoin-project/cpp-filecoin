addtest(filecoin_ipfs_merkledag_service_test
    ipfs_merkledag_service_test.cpp
    )
target_link_libraries(filecoin_ipfs_merkledag_service_test
    filecoin_ipfs_merkledag_service
    filecoin_buffer
    )
