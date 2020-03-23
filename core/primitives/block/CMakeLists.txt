#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(block INTERFACE)
target_link_libraries(block INTERFACE
    address
    cbor
    signature
    tickets
    ipld_block
    )
