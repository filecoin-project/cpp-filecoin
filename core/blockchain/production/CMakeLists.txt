#
#Copyright Soramitsu Co., Ltd.All Rights Reserved.
#SPDX - License - Identifier : Apache - 2.0
#

filecoin_add_library(filecoin_block_producer
    impl/block_producer_impl.cpp
    )
target_link_libraries(filecoin_block_producer
    filecoin_bls_provider
    filecoin_outcome
    filecoin_buffer
    filecoin_address
    filecoin_clock
    filecoin_interpreter
    filecoin_tipset
    filecoin_ipfs_datastore_in_memory
    )
