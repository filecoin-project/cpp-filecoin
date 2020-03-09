#
#Copyright Soramitsu Co., Ltd.All Rights Reserved.
#SPDX - License - Identifier : Apache - 2.0
#

add_library(block_producer
    impl/block_producer_impl.cpp
    )
target_link_libraries(block_producer
    bls_provider
    outcome
    buffer
    address
    clock
    interpreter
    tipset
    ipfs_datastore_in_memory
    )
