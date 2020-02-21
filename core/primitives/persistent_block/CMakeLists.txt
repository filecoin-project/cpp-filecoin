# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(persistent_block
    persistent_block.cpp
    )
target_link_libraries(persistent_block
    buffer
    cbor
    cid
    )
