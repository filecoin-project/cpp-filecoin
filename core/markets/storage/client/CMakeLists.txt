#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_market_client
    impl/storage_market_client_impl.cpp
    )
target_link_libraries(storage_market_client
    address
    blob
    data_transfer
    discovery
    fuhon_fsm
    outcome
    logger
    piece
    signature
    market_actor_v0
    message
    tipset
    )
