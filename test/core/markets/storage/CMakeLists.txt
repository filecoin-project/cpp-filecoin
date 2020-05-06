#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(provider)

addtest(storage_market_test
    storage_market_types_encoding_test.cpp
    )
target_link_libraries(storage_market_test
    address
    cbor
    piece
    )
