#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(retrieval_market_protocol_test
    retrieval_protocol_v0_0_1_test.cpp
    retrieval_protocol_v1_0_0_test.cpp
    query_protocol_v0_0_1_test.cpp
    query_protocol_v1_0_0_test.cpp
    )
target_link_libraries(retrieval_market_protocol_test
    address
    cbor
    )
