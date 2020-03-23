#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(amt
    amt.cpp
    )
target_link_libraries(amt
    buffer
    cbor
    cid
    outcome
    ipld_block
    )
