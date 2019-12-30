#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(message
    message.cpp
    message_util.cpp
    impl/message_signer_bls.cpp
    impl/message_signer_secp256k1.cpp
    )

target_link_libraries(message
    Boost::boost
    address
    outcome
    )
