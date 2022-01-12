/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/wallet/wallet_api.hpp"

namespace fc::api {

  // TODO(m.tagirov) extend MultiWallet to use LedgerWallet

  // MultiWallet is used as one wallet for many wallet implementations
  class MultiWallet : public WalletApi {
   public:
    explicit MultiWallet(std::shared_ptr<WalletApi> localWallet)
        : localWallet(std::move(localWallet)) {}

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
    std::shared_ptr<WalletApi> localWallet;
  };

}  // namespace fc::api
