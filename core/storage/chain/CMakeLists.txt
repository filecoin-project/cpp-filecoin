# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(msg_waiter
    msg_waiter.cpp
    )
target_link_libraries(msg_waiter
    message
    )

add_library(receipt_loader
    receipt_loader.cpp
    )
target_link_libraries(receipt_loader
    message
    )
