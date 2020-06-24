#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(paychannel_manager
    impl/paychannel_manager_impl.cpp
    impl/paychannel_manager_error.cpp
    )
target_link_libraries(paychannel_manager
    address
    todo_error
    state_tree
    )
