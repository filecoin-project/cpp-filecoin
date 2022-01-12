/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/wallet/multi_wallet.hpp"

namespace fc::api {

  outcome::result<TokenAmount> MultiWallet::WalletBalance(
      const TipsetContext &context, const Address &address) const {
    return localWallet->WalletBalance(context, address);
  }

  outcome::result<Address> MultiWallet::WalletDefaultAddress() const {
    return localWallet->WalletDefaultAddress();
  }

  outcome::result<bool> MultiWallet::WalletHas(const TipsetContext &context,
                                               const Address &address) const {
    return localWallet->WalletHas(context, address);
  }

  outcome::result<Address> MultiWallet::WalletImport(
      const KeyInfo &info) const {
    return localWallet->WalletImport(info);
  }

  outcome::result<Address> MultiWallet::WalletNew(
      const std::string &type) const {
    return localWallet->WalletNew(type);
  }

  outcome::result<void> MultiWallet::WalletSetDefault(
      const Address &address) const {
    return localWallet->WalletSetDefault(address);
  }

  outcome::result<Signature> MultiWallet::WalletSign(
      const TipsetContext &context,
      const Address &address,
      const Bytes &data) const {
    return localWallet->WalletSign(context, address, data);
  }

  outcome::result<bool> MultiWallet::WalletVerify(
      const TipsetContext &context,
      const Address &address,
      const Bytes &data,
      const Signature &signature) const {
    return localWallet->WalletVerify(context, address, data, signature);
  }

}  // namespace fc::api
