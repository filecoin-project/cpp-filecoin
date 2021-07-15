#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(block_validator_test
    block_validator_test.cpp
    )
target_link_libraries(block_validator_test
    block_validator
    miner_actor_v0
    miner_actor_v2
    miner_actor_v3
    )
