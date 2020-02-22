# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(filecoin_chain_epoch_codec
    chain_epoch_codec.cpp
    )
target_link_libraries(filecoin_chain_epoch_codec
    p2p::p2p_uvarint
    )
