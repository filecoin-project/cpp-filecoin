#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(storage_fsm)

add_library(miner
    mining.cpp
    windowpost.cpp
    )
target_link_libraries(miner
    cbor
    message
    rle_plus_codec
    tipset
    )
