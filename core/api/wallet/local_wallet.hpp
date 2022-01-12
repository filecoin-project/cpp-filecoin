/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/wallet/wallet_api.hpp"

#include "storage/keystore/keystore.hpp"
#include "storage/map_prefix/prefix.hpp"

namespace fc::api {
  using storage::OneKey;
  using storage::keystore::KeyStore;

  class LocalWallet : public WalletApi {
   public:
    LocalWallet(const std::shared_ptr<KeyStore> &key_store,
                const std::shared_ptr<OneKey> &wallet_default_address)
        : key_store(key_store),
          wallet_default_address(wallet_default_address) {}

    outcome::result<TokenAmount> WalletBalance(
        const TipsetContext &context, const Address &address) const override;

    outcome::result<Address> WalletDefaultAddress() const override;

    outcome::result<bool> WalletHas(const TipsetContext &context,
                                    const Address &address) const override;

    outcome::result<Address> WalletImport(const KeyInfo &info) const override;

    outcome::result<Address> WalletNew(const std::string &type) const override;

    outcome::result<void> WalletSetDefault(
        const Address &address) const override;

    outcome::result<Signature> WalletSign(const TipsetContext &context,
                                          const Address &address,
                                          const Bytes &data) const override;

    outcome::result<bool> WalletVerify(
        const TipsetContext &context,
        const Address &address,
        const Bytes &data,
        const Signature &signature) const override;

   private:
    std::shared_ptr<KeyStore> key_store;
    std::shared_ptr<OneKey> wallet_default_address;
  };

}  // namespace fc::api
