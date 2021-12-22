/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ledger_admin_hid.hpp"

#include "ledger_device_hid.hpp"

namespace ledger {

  int LedgerAdminHid::CountDevices() const {
    // todo
    return 0;
  }

  std::tuple<std::string, Error> LedgerAdminHid::ListDevices() const {
    // todo
    return std::make_tuple("", Error{});
  }

  std::tuple<std::shared_ptr<LedgerDevice>, Error> LedgerAdminHid::Connect(
      int device_index) const {
    // todo
    return std::make_tuple(std::make_shared<LedgerDeviceHid>(), Error{});
  }

}  // namespace ledger
