/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <tuple>
#include "cpp-ledger/common/types.hpp"

namespace ledger {

  class LedgerDevice {
   public:
    virtual ~LedgerDevice() = default;

    virtual std::tuple<Bytes, Error> Exchange(const Bytes &command) const = 0;
    virtual void Close() = 0;
  };

  class LedgerAdmin {
   public:
    virtual ~LedgerAdmin() = default;

    virtual int CountDevices() const = 0;
    virtual std::tuple<std::string, Error> ListDevices() const = 0;
    virtual std::tuple<std::shared_ptr<LedgerDevice>, Error> Connect(
        int deviceIndex) const = 0;
  };

  std::shared_ptr<LedgerAdmin> CreateLedgerAdmin();

}  // namespace ledger
