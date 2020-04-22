#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(cbor_stream
    cbor_buffering.cpp
    cbor_stream.cpp
    )
target_link_libraries(cbor_stream
    cbor
    )
