#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multisig_actor_v2
    impl/multisig_utils_impl_v2.cpp
    multisig_actor.cpp
    )
target_link_libraries(multisig_actor_v2
    actor
    outcome
    )
