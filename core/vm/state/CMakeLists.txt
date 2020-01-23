#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(state_tree
    impl/state_tree_impl.cpp
    )
target_link_libraries(state_tree
    actor
    Boost::boost
    )
