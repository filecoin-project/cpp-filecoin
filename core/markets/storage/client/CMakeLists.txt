#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_market_client
    impl/storage_market_client_impl.cpp
    impl/client_data_transfer_request_validator.cpp
    impl/client_state_store.cpp
    )
target_link_libraries(storage_market_client
    address
    blob
    data_transfer_graphsync
    discovery
    fuhon_fsm
    outcome
    logger
    piece
    signature
    market_actor
    message
    tipset
    )
