/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/ledger/device_hid_info.hpp"

#include <hidapi/hidapi.h>
#include <iomanip>
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
    ss << "VendorID      : " << std::setfill('0') << std::setw(2) << std::hex
       << int(vendorId) << std::endl;
    ss << "ProductID     : " << std::setfill('0') << std::setw(2) << std::hex
       << int(productId) << std::endl;
    ss << "Release       : " << std::setfill('0') << std::setw(2) << std::hex
       << int(releaseNumber) << std::endl;

    ss << "Serial        : ";
    for (wchar_t symbol : serialNumber) {
      ss << std::setfill('0') << std::setw(2) << std::hex << int(symbol);
    }
    ss << std::endl;

    ss << "Manufacturer  : " << convertToString(manufacturerString)
       << std::endl;
    ss << "Product       : " << convertToString(productString) << std::endl;
    ss << "UsagePage     : " << std::setfill('0') << std::setw(2) << std::hex
       << int(usagePage) << std::endl;
    ss << "Usage         : " << std::setfill('0') << std::setw(2) << std::hex
       << int(usage) << std::endl;
    return ss.str();
  }

}  // namespace ledger
