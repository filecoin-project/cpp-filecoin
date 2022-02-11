/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cpp-ledger/ledger/ledger.hpp"

namespace ledger {

  class LedgerAdminHid : public LedgerAdmin {
   public:
    int CountDevices() const override;
    std::tuple<std::string, Error> ListDevices() const override;
    std::tuple<std::shared_ptr<LedgerDevice>, Error> Connect(
        int deviceIndex) const override;
  };

}  // namespace ledger
