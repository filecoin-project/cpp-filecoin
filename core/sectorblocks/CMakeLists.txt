#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(sectorblocks
        impl/blocks_impl.cpp
        )

target_link_libraries(sectorblocks
        outcome
        miner
        piece
        logger
        leveldb
        )
