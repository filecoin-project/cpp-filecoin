#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_miner_actor_test
    miner_actor_test.cpp
    )
target_link_libraries(filecoin_miner_actor_test
    filecoin_miner_actor
    )
