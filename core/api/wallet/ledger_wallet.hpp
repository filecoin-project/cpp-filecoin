/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/wallet/wallet_api.hpp"

#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "storage/map_prefix/prefix.hpp"

namespace fc::api {
  using crypto::secp256k1::Secp256k1ProviderDefault;
  using storage::MapPtr;

  class LedgerWallet {
   public:
    static void fillLedgerWalletApi(const std::shared_ptr<WalletApi> &api,
                                    const MapPtr &store);
  };

}  // namespace fc::api
