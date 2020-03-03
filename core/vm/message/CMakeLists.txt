#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(message
    message.cpp
    message_util.cpp
    impl/message_signer_impl.cpp
    )

target_link_libraries(message
    Boost::boost
    address
    buffer
    logger
    keystore
    outcome
    signature
    ipld_block
    )
