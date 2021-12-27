/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/ledger/device_hid_info.hpp"

#include <hidapi/hidapi.h>
#include <sstream>
#include "cpp-ledger/ledger/utils.hpp"

namespace ledger {

  DeviceHidInfo::DeviceHidInfo(const hid_device_info *deviceInfo) {
    if (deviceInfo != nullptr) {
      path = std::string(deviceInfo->path);
      vendorId = deviceInfo->vendor_id;
      productId = deviceInfo->product_id;
      serialNumber = std::wstring(deviceInfo->serial_number);
      releaseNumber = deviceInfo->release_number;
      manufacturerString = std::wstring(deviceInfo->manufacturer_string);
      productString = std::wstring(deviceInfo->product_string);
      usagePage = deviceInfo->usage_page;
      usage = deviceInfo->usage;
      interfaceNumber = deviceInfo->interface_number;
    }
  }

  std::string DeviceHidInfo::ToString() const {
    std::stringstream ss;
    ss << "============ " << path << std::endl;
    ss << "VendorID      : " << std::hex << vendorId << std::endl;
    ss << "ProductID     : " << std::hex << productId << std::endl;
    ss << "Release       : " << std::hex << releaseNumber << std::endl;

    ss << "Serial        : ";
    for (wchar_t symbol : serialNumber) {
      ss << std::hex << symbol;
    }
    ss << std::endl;

    ss << "Manufacturer  : " << convertToString(manufacturerString)
       << std::endl;
    ss << "Product       : " << convertToString(productString) << std::endl;
    ss << "UsagePage     : " << std::hex << usagePage << std::endl;
    ss << "Usage         : " << std::hex << usage << std::endl;
    return ss.str();
  }

}  // namespace ledger
