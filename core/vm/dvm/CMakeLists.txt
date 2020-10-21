#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(dvm
    dvm.cpp
    )
target_link_libraries(dvm
    buffer
    cbor
    logger
    message
    state_tree
    )
