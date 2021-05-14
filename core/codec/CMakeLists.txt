#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(cbor)
add_subdirectory(json)
add_subdirectory(rle)

add_library(cbor_types
    cbor/cbor_types.cpp
    )
target_link_libraries(cbor_types
    address
    signature
    )
