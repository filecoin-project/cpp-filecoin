# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_persistent_block_test
    persistent_block_test.cpp
    )
target_link_libraries(filecoin_persistent_block_test
    filecoin_persistent_block
    )
