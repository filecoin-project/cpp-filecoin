#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(message
    message.cpp
    impl/message_signer_impl.cpp
    )

target_link_libraries(message
    Boost::boost
    address
    logger
    keystore
    outcome
    signature
    )
