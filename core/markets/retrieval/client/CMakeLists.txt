#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(retrieval_market_client
    impl/retrieval_client_impl.cpp
    impl/retrieval_client_error.cpp
    )
target_link_libraries(retrieval_market_client
    address
    logger
    cbor
    cbor_stream
    ipld_verifier
    )
