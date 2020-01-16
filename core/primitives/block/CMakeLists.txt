#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(block
    block.cpp
    )
target_link_libraries(block
    address
    cbor
    tickets
    )
