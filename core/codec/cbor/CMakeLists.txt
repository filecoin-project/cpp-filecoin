#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(cbor
    cbor_decode_stream.cpp
    cbor_dump.cpp
    cbor_encode_stream.cpp
    cbor_errors.cpp
    cbor_resolve.cpp
    )
target_link_libraries(cbor
    cid
    outcome
    )
