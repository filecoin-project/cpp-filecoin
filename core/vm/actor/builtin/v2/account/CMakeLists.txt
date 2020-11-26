#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(account_actor_v2
    account_actor.cpp
    )
target_link_libraries(account_actor_v2
    actor
    outcome
    )
