/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/wallet/wallet_api.hpp"
#include "storage/map_prefix/prefix.hpp"

namespace fc::api {
  using storage::MapPtr;

  class LedgerWallet {
   public:
    static void fillLedgerWalletApi(const std::shared_ptr<WalletApi> &api,
                                    const MapPtr &store);
  };

}  // namespace fc::api
