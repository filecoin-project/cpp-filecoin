#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_reward_actor
    reward_actor.cpp
    )
target_link_libraries(filecoin_reward_actor
    Boost::boost
    filecoin_actor
    filecoin_address
    filecoin_exit_code
    filecoin_outcome
    )
