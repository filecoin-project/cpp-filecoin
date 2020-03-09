# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(filecoin_persistent_block
    persistent_block.cpp
    )
target_link_libraries(filecoin_persistent_block
    filecoin_buffer
    filecoin_cbor
    filecoin_cid
    )
