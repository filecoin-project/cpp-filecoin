/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/wallet/local_wallet.hpp"

#include "common/error_text.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"

namespace fc::api {

  outcome::result<TokenAmount> LocalWallet::WalletBalance(
      const TipsetContext &context, const Address &address) const {
    OUTCOME_TRY(actor, context.state_tree.tryGet(address));
    if (actor) {
      return actor->balance;
    }
    return 0;
  }

  outcome::result<Address> LocalWallet::WalletDefaultAddress() const {
    if (!wallet_default_address->has()) {
      return ERROR_TEXT("WalletDefaultAddress: default wallet is not set");
    }
    return wallet_default_address->getCbor<Address>();
  }

  outcome::result<bool> LocalWallet::WalletHas(const TipsetContext &context,
                                               const Address &address) const {
    Address resolvedAddr = address;
    if (!address.isKeyType()) {
      OUTCOME_TRYA(resolvedAddr, context.accountKey(address));
    }
    return key_store->has(resolvedAddr);
  }

  outcome::result<Address> LocalWallet::WalletImport(
      const KeyInfo &info) const {
    return key_store->put(info.type, info.private_key);
  }

  outcome::result<Address> LocalWallet::WalletNew(
      const std::string &type) const {
    Address address;
    if (type == "bls") {
      OUTCOME_TRYA(address,
                   key_store->put(crypto::signature::Type::kBls,
                                  crypto::bls::BlsProviderImpl{}
                                      .generateKeyPair()
                                      .value()
                                      .private_key));
    } else if (type == "secp256k1") {
      OUTCOME_TRYA(address,
                   key_store->put(crypto::signature::Type::kSecp256k1,
                                  crypto::secp256k1::Secp256k1ProviderImpl{}
                                      .generate()
                                      .value()
                                      .private_key));
    } else {
      return ERROR_TEXT("WalletNew: unknown type");
    }
    if (!wallet_default_address->has()) {
      wallet_default_address->setCbor(address);
    }
    return std::move(address);
  }

  outcome::result<void> LocalWallet::WalletSetDefault(
      const Address &address) const {
    wallet_default_address->setCbor(address);
    return outcome::success();
  }

  outcome::result<Signature> LocalWallet::WalletSign(
      const TipsetContext &context,
      const Address &address,
      const Bytes &data) const {
    Address resolvedAddr = address;
    if (!address.isKeyType()) {
      OUTCOME_TRYA(resolvedAddr, context.accountKey(address));
    }
    return key_store->sign(resolvedAddr, data);
  }

  outcome::result<bool> LocalWallet::WalletVerify(
      const TipsetContext &context,
      const Address &address,
      const Bytes &data,
      const Signature &signature) const {
    Address resolvedAddr = address;
    if (!address.isKeyType()) {
      OUTCOME_TRYA(resolvedAddr, context.accountKey(address));
    }
    return key_store->verify(resolvedAddr, data, signature);
  }

}  // namespace fc::api
