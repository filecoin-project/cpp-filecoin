/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <hidapi.h>
#include <functional>
#include <memory>
#include <mutex>
#include "cpp-ledger/common/types.hpp"
#include "cpp-ledger/ledger/device_hid_info.hpp"

namespace ledger {

  class DeviceHid {
   public:
    DeviceHid() = default;
    ~DeviceHid() = default;

    DeviceHid(DeviceHid &&other) noexcept;
    DeviceHid &operator=(DeviceHid &&other) noexcept;

    DeviceHid(const DeviceHid &) = delete;
    DeviceHid &operator=(const DeviceHid &) = delete;

    void SetInfo(const hid_device_info *deviceInfo);

    Error Open();
    void Close();
    std::tuple<int, Error> Write(const Bytes &bytes) const;
    std::tuple<int, Error> Read(Bytes &bytes) const;

    bool IsLedgerDevice() const;
    std::string ToString() const;

   private:
    using DeviceDeleter = std::function<void(hid_device *)>;

    DeviceHidInfo info;
    std::unique_ptr<hid_device, DeviceDeleter> device;
    mutable std::mutex mutex;
  };

  std::vector<DeviceHid> Enumerate(uint16_t vendorId, uint16_t productId);

}  // namespace ledger
