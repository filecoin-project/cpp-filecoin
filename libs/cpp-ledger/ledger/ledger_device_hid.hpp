/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <hidapi/hidapi.h>

#include "ledger.hpp"

#include "device_hid.hpp"

namespace ledger {

  class LedgerDeviceHid : public LedgerDevice {
   public:
    LedgerDeviceHid(DeviceHid &&deviceHid) : device(std::move(deviceHid)) {}

    LedgerDeviceHid(LedgerDeviceHid &&other) = default;
    LedgerDeviceHid &operator=(LedgerDeviceHid &&other) = default;

    LedgerDeviceHid(const LedgerDeviceHid &) = delete;
    LedgerDeviceHid &operator=(const LedgerDeviceHid &) = delete;

    std::tuple<Bytes, Error> Exchange(const Bytes &command) const override;
    void Close() override;

   private:
    std::tuple<int, Error> Write(const Bytes &bytes) const;
    std::vector<Bytes> Read() const;

    DeviceHid device;
  };

}  // namespace ledger
