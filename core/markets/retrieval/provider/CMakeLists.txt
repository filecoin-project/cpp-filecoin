#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(retrieval_market_provider
    impl/retrieval_provider_impl.cpp
    query_responder/query_responder_impl.cpp
    )
target_link_libraries(retrieval_market_provider
    address
    logger
    cbor
    cbor_stream
    )
