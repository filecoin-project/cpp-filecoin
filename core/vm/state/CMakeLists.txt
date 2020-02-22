#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_state_tree
    impl/state_tree_impl.cpp
    )
target_link_libraries(filecoin_state_tree
    filecoin_actor
    Boost::boost
    filecoin_hamt
    )
