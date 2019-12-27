#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(state_tree
    state_tree.cpp
    )
target_link_libraries(state_tree
    actor
    Boost::boost
    )
