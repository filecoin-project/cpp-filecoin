/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/wallet/local_wallet.hpp"

#include "common/error_text.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"
#include "primitives/address/address_codec.hpp"

namespace fc::api {

  void LocalWallet::fillLocalWalletApi(
      const std::shared_ptr<WalletApi> &api,
      const std::shared_ptr<KeyStore> &key_store,
      const std::function<outcome::result<TipsetContext>(
          const TipsetKey &tipset_key, bool interpret)> &ts_ctx,
      const std::shared_ptr<OneKey> &wallet_default_address) {
    api->WalletBalance = [=](auto &address) -> outcome::result<TokenAmount> {
      OUTCOME_TRY(context, ts_ctx({}, false));
      OUTCOME_TRY(actor, context.state_tree.tryGet(address));
      if (actor) {
        return actor->balance;
      }
      return 0;
    };
    api->WalletDefaultAddress = [=]() -> outcome::result<Address> {
      if (!wallet_default_address->has())
        return ERROR_TEXT("WalletDefaultAddress: default wallet is not set");
      return wallet_default_address->getCbor<Address>();
    };
    api->WalletHas = [=](auto address) -> outcome::result<bool> {
      if (!address.isKeyType()) {
        OUTCOME_TRY(context, ts_ctx({}, false));
        OUTCOME_TRYA(address, context.accountKey(address));
      }
      return key_store->has(address);
    };
    api->WalletImport = {[=](auto &info) -> outcome::result<Address> {
      OUTCOME_TRY(private_key, info.getPrivateKey());
      return key_store->put(info.type, private_key);
    }};
    api->WalletNew = {[=](auto &type) -> outcome::result<Address> {
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
    }};
    api->WalletList = [=]() -> outcome::result<std::vector<Address>> {
      OUTCOME_TRY(addresses, key_store->list());
      std::sort(addresses.begin(), addresses.end());
      auto last = std::unique(addresses.begin(), addresses.end());
      addresses.erase(last, addresses.end());
      return addresses;
    };
    api->WalletSetDefault = [=](auto &address) -> outcome::result<void> {
      wallet_default_address->setCbor(address);
      return outcome::success();
    };
    api->WalletSign = [=](auto address,
                          auto data) -> outcome::result<Signature> {
      if (!address.isKeyType()) {
        OUTCOME_TRY(context, ts_ctx({}, false));
        OUTCOME_TRYA(address, context.accountKey(address));
      }
      return key_store->sign(address, data);
    };
    api->WalletVerify =
        [=](auto address, auto data, auto signature) -> outcome::result<bool> {
      if (!address.isKeyType()) {
        OUTCOME_TRY(context, ts_ctx({}, false));
        OUTCOME_TRYA(address, context.accountKey(address));
      }
      return key_store->verify(address, data, signature);
    };
    api->WalletDelete = [=](auto &address) -> outcome::result<void> {
      OUTCOME_TRY(has, key_store->has(address));
      if (has) {
        OUTCOME_TRY(key_store->remove(address));
        return outcome::success();
      }
      return ERROR_TEXT("WalletDelete: Address does not exist");
    };
  }

}  // namespace fc::api
