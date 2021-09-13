#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(import_manager)

add_library(storage_market_client
    impl/storage_market_client_impl.cpp
    )
target_link_libraries(storage_market_client
    address
    blob
    data_transfer
    discovery
    outcome
    logger
    piece
    signature
    message
    tipset
    memory_indexed_car
    storage_market_import_manager
    )
