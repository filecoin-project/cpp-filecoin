#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_market_client
    client_impl.cpp
    client_fsm_transitions.cpp
    )
target_link_libraries(storage_market_client
    blob
    outcome
    logger
    )
