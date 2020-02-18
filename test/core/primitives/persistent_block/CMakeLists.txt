# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(persistent_block_test
    persistent_block_test.cpp
    )
target_link_libraries(persistent_block_test
    persistent_block
    )
