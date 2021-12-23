/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <hidapi/hidapi.h>

#include <mutex>

#include "common.hpp"

namespace ledger {

  struct DeviceHid {
    DeviceHid();
    ~DeviceHid();

    DeviceHid(DeviceHid &&other);
    DeviceHid &operator=(DeviceHid &&other);

    DeviceHid(const DeviceHid &) = delete;
    DeviceHid &operator=(const DeviceHid &) = delete;

    void SetInfo(const hid_device_info *device_info);

    Error Open();
    void Close();
    std::tuple<int, Error> Write(const Bytes &bytes) const;
    std::tuple<int, Error> Read(Bytes &bytes) const;

    bool IsLedgerDevice() const;

    hid_device_info info;
    hid_device *device;

   private:
    mutable std::mutex mutex;
  };

  std::vector<DeviceHid> Enumerate(uint16_t vendorId, uint16_t productId);

}  // namespace ledger
