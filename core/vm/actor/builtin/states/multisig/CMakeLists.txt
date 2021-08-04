#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multisig_actor_state
    multisig_actor_state.cpp
    bind_states.cpp
    )
target_link_libraries(multisig_actor_state
    cbor
    multisig_types
    )
