#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(retrieval_market_test
    retrieval_market_test.cpp
    )
target_link_libraries(retrieval_market_test
    retrieval_market_provider
    retrieval_market_client
    piece_storage
    in_memory_storage
    ipfs_datastore_in_memory
    p2p::p2p
    p2p::p2p_literals
    )
