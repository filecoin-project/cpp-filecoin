#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(actor_state
    state_provider.cpp
    state_manager.cpp
    )
target_link_libraries(actor_state
    actor
    outcome
    )
