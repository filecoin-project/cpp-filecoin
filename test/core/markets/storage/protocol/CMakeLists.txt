#ask_protocol_named_cbor_test
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(storage_market_protocol_named_cbor_test
    ask_protocol_named_cbor_test.cpp
    mk_protocol_named_cbor_test.cpp
    status_protocol_named_cbor_test.cpp
    )
target_link_libraries(storage_market_protocol_named_cbor_test
    address
    cbor
    signature
    piece
    )
