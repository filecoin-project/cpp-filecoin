/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/filecoin/ledger_filecoin.hpp"

namespace ledger::filecoin {

  void LedgerFilecoinManager::ListFilecoinDevices(
      const std::vector<uint32_t> &path) const {
    // TODO (m.tagirov) impplement
  }

  std::tuple<std::shared_ptr<LedgerFilecoin>, Error>
  LedgerFilecoinManager::ConnectLedgerFilecoinApp(
      const std::string &seekingAddress,
      const std::vector<uint32_t> &path) const {
    // TODO (m.tagirov) impplement
    return std::make_tuple(nullptr, Error{"not implemented"});
  }

  std::tuple<std::shared_ptr<LedgerFilecoin>, Error>
  LedgerFilecoinManager::FindLedgerFilecoinApp() const {
    // TODO (m.tagirov) impplement
    return std::make_tuple(nullptr, Error{"not implemented"});
  }

}  // namespace ledger::filecoin
