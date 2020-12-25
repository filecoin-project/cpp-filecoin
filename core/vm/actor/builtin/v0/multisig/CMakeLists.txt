#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multisig_actor_v0
    multisig_actor_state.cpp
    impl/multisig_utils_impl_v0.cpp
    multisig_actor.cpp
    )
target_link_libraries(multisig_actor_v0
    actor
    outcome
    )
