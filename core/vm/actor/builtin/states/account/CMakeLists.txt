#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(account_actor_state
    bind_states.cpp
    )
target_link_libraries(account_actor_state
    cbor
    )
