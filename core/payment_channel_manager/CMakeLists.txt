#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(payment_channel_manager
    impl/payment_channel_manager_impl.cpp
    impl/payment_channel_manager_error.cpp
    )
target_link_libraries(payment_channel_manager
    address
    todo_error
    state_tree
    )
