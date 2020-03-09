#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_message_pool_test
    gas_price_scored_message_storage_test.cpp
    )
target_link_libraries(filecoin_message_pool_test
    filecoin_message_pool
    filecoin_message_test_util
    )
