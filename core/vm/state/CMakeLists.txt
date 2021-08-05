#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(state_tree
    state_tree_error.cpp
    impl/state_tree_impl.cpp
    )
target_link_libraries(state_tree
    address_key
    Boost::boost
    dvm
    hamt
    init_actor_state
    )
