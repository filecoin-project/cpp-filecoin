#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_block_producer_test
    block_producer_test.cpp
    )
target_link_libraries(filecoin_block_producer_test
    filecoin_block_producer
    )
