#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_rle_bitset INTERFACE)
target_link_libraries(filecoin_rle_bitset INTERFACE
    filecoin_cbor
    filecoin_rle_plus_codec
    )
