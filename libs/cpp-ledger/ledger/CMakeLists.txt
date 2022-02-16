#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

find_library(HIDAPI_LIB
    NAMES hidapi hidapi-libusb)

find_path(HIDAPI_INCLUDE_DIR
    NAMES hidapi.h
    PATH_SUFFIXES
    hidapi)

add_library(ledger_device
    apdu_wrapper.cpp
    device_hid_info.cpp
    device_hid.cpp
    ledger_device_hid.cpp
    ledger_admin_hid.cpp
    utils.cpp
    )

target_include_directories(ledger_device PRIVATE ${HIDAPI_INCLUDE_DIR})
target_link_libraries(ledger_device ${HIDAPI_LIB})
