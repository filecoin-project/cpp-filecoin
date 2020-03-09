#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_message
    message.cpp
    message_util.cpp
    impl/message_signer_impl.cpp
    )

target_link_libraries(filecoin_message
    Boost::boost
    filecoin_address
    filecoin_buffer
    filecoin_logger
    filecoin_keystore
    filecoin_outcome
    filecoin_signature
    filecoin_ipld_block
    )
