#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

find_package(hidapi REQUIRED)

add_library(ledger_device
    apdu_wrapper.cpp
    device_hid_info.cpp
    device_hid.cpp
    ledger_device_hid.cpp
    ledger_admin_hid.cpp
    utils.cpp
    )

find_path(HIDAPI_INCLUDE_DIR
	NAMES hidapi.h
	PATH_SUFFIXES
	hidapi)

target_include_directories(ledger_device PRIVATE ${HIDAPI_INCLUDE_DIR})

if(TARGET hidapi::libusb)
    target_link_libraries(ledger_device INTERFACE hidapi::libusb)
else()
    target_link_libraries(ledger_device INTERFACE hidapi::hidapi)
endif()
