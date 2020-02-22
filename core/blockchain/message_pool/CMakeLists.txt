#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_message_pool
    impl/gas_price_scored_message_storage.cpp
    impl/message_pool_error.cpp)
target_link_libraries(filecoin_message_pool
    filecoin_outcome
    filecoin_message
    )
