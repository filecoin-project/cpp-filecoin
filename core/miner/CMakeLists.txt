#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(miner
    windowpost.cpp
    )
target_link_libraries(miner
    cbor
    )
