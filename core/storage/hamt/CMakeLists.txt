#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(hamt
    hamt_walk.cpp
    )
target_link_libraries(hamt
    blob
    cbor
    cid
    outcome
    )
