/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/wallet/wallet_api.hpp"

#include "api/types/tipset_context.hpp"
#include "storage/keystore/keystore.hpp"
#include "storage/map_prefix/prefix.hpp"

namespace fc::api {
  using storage::OneKey;
  using storage::keystore::KeyStore;

  class LocalWallet {
   public:
    static void fillLocalWalletApi(
        const std::shared_ptr<WalletApi> &api,
        const std::shared_ptr<KeyStore> &key_store,
        const std::function<outcome::result<TipsetContext>(
            const TipsetKey &tipset_key, bool interpret)> &ts_ctx,
        const std::shared_ptr<OneKey> &wallet_default_address);
  };

}  // namespace fc::api
