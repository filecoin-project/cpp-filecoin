#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(rle_bitset INTERFACE)
target_link_libraries(rle_bitset INTERFACE
    cbor
    rle_plus_codec
    )
