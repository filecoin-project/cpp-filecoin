#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(ledger_device
    apdu_wrapper.cpp
    device_hid_info.cpp
    device_hid.cpp
    ledger_device_hid.cpp
    ledger_admin_hid.cpp
    utils.cpp
    )
target_link_libraries(ledger_device
    hidapi
    )
