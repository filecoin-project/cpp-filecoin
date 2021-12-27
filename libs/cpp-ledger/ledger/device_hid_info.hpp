/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>

// Forward declaration
struct hid_device_info;

namespace ledger {

  struct DeviceHidInfo {
    std::string path{};
    uint16_t vendorId{};
    uint16_t productId{};
    std::wstring serialNumber;
    uint16_t releaseNumber{};
    std::wstring manufacturerString;
    std::wstring productString;
    uint16_t usagePage{};
    uint16_t usage{};
    int interfaceNumber{};

    DeviceHidInfo() = default;
    explicit DeviceHidInfo(const hid_device_info *deviceInfo);
    ~DeviceHidInfo() = default;

    std::string ToString() const;
  };

}  // namespace ledger
