#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(message_pool_test
    gas_price_scored_message_storage_test.cpp
    )
target_link_libraries(message_pool_test
    message_pool
    message_test_util
    )
