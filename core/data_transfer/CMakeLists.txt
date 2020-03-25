#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(data_transfer
    message_receiver.cpp
    message.cpp
    )
target_link_libraries(data_transfer
    p2p::p2p
    logger
    )
