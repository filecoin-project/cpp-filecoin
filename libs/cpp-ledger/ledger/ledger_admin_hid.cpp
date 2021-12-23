/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/ledger/ledger_admin_hid.hpp"

#include <iostream>
#include "cpp-ledger/ledger/const.hpp"
#include "cpp-ledger/ledger/ledger_device_hid.hpp"
#include "cpp-ledger/ledger/utils.hpp"

namespace ledger {

  int LedgerAdminHid::CountDevices() const {
    const auto devices = Enumerate(0, 0);

    int count = 0;

    for (const auto &device : devices) {
      if (device.IsLedgerDevice()) {
        count++;
      }
    }

    return count;
  }

  std::tuple<std::string, Error> LedgerAdminHid::ListDevices() const {
    const auto devices = Enumerate(0, 0);

    if (devices.empty()) {
      std::cout << "No devices" << std::endl;
    }

    for (const auto &device : devices) {
      std::cout << "============ " << device.info.path << std::endl;
      std::cout << "VendorID      : " << std::hex << device.info.vendor_id
                << std::endl;
      std::cout << "ProductID     : " << std::hex << device.info.product_id
                << std::endl;
      std::cout << "Release       : " << std::hex << device.info.release_number
                << std::endl;

      std::cout << "Serial        : ";
      std::wstring serial(device.info.serial_number);
      for (wchar_t symbol : serial) {
        std::cout << std::hex << symbol;
      }
      std::cout << std::endl;

      std::cout << "Manufacturer  : "
                << convertToString(device.info.manufacturer_string)
                << std::endl;
      std::cout << "Product       : "
                << convertToString(device.info.product_string) << std::endl;
      std::cout << "UsagePage     : " << std::hex << device.info.usage_page
                << std::endl;
      std::cout << "Usage         : " << std::hex << device.info.usage
                << std::endl;
      std::cout << std::endl;
    }

    return std::make_tuple("", Error{});
  }

  std::tuple<std::shared_ptr<LedgerDevice>, Error> LedgerAdminHid::Connect(
      int deviceIndex) const {
    auto devices = Enumerate(kVendorLedger, 0);

    int currentIndex = 0;
    for (auto &device : devices) {
      if (device.IsLedgerDevice()) {
        if (currentIndex == deviceIndex) {
          const auto err = device.Open();
          if (err != std::nullopt) {
            return std::make_tuple(nullptr, err);
          }

          auto ledgerDevice =
              std::make_shared<LedgerDeviceHid>(std::move(device));
          return std::make_tuple(ledgerDevice, Error{});
        }
        currentIndex++;
        if (currentIndex > deviceIndex) {
          break;
        }
      }
    }

    return std::make_tuple(
        nullptr,
        Error{"LedgerHID device (id " + std::to_string(deviceIndex)
              + ") not found"});
  }

}  // namespace ledger
