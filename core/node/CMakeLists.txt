#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(node
    blocksync.cpp
    hello.cpp
    peermgr.cpp
    sync.cpp
    )
target_link_libraries(node
    cbor_stream
    )
