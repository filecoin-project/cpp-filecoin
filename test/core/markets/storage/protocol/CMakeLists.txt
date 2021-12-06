#ask_protocol_named_cbor_test
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(storage_market_protocol_named_cbor_test
    ask_protocol_v1_0_1_named_cbor_test.cpp
    ask_protocol_v1_1_0_named_cbor_test.cpp
    mk_protocol_v1_0_1_named_cbor_test.cpp
    mk_protocol_v1_1_0_named_cbor_test.cpp
    status_protocol_v1_0_1_named_cbor_test.cpp
    status_protocol_v1_1_0_named_cbor_test.cpp
    )
target_link_libraries(storage_market_protocol_named_cbor_test
    address
    bls_provider
    cbor
    signature
    piece
    )
