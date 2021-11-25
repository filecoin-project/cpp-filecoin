#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(ask_protocol_cbor
    ask_protocol_named_cbor.cpp
    )
target_link_libraries(ask_protocol_cbor
    address
    cbor
    signature
    piece
    )
