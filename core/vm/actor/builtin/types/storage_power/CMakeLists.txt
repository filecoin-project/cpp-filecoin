#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_power_types
    bind_types.cpp
    )
target_link_libraries(storage_power_types
    cbor
    )
