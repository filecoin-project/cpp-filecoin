#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_block INTERFACE)
target_link_libraries(filecoin_block INTERFACE
    filecoin_address
    filecoin_cbor
    filecoin_signature
    filecoin_tickets
    )
