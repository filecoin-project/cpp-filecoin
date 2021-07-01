#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(universal_types INTERFACE)
target_link_libraries(universal_types INTERFACE
    cbor
    cid
    runtime
    )
