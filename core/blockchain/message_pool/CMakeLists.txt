#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(message_pool
    impl/gas_price_scored_message_storage.cpp
    )
target_link_libraries(message_pool
    outcome
    message
    )
