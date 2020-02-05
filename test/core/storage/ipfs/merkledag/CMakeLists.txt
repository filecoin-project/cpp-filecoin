addtest(ipfs_merkledag_service_test
    ipfs_merkledag_service_test.cpp
    )
target_link_libraries(ipfs_merkledag_service_test
    ipfs_merkledag_service
    buffer
    )
