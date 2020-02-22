#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_miner_actor
    miner_actor.cpp
    )
target_link_libraries(filecoin_miner_actor
    filecoin_address
    filecoin_cbor
    filecoin_chain_epoch_codec
    filecoin_rle_bitset
    )
