/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/wallet/ledger_wallet.hpp"

#include "api/rpc/json.hpp"
#include "api/types/ledger_key_info.hpp"
#include "api/wallet/ledger.hpp"
#include "codec/json/json.hpp"

namespace fc::api {

  void LedgerWallet::fillLedgerWalletApi(const std::shared_ptr<WalletApi> &api,
                                         const MapPtr &store) {
    api->WalletHas =
        [=, prev = api->WalletHas](auto address) -> outcome::result<bool> {
      if (address.isSecp256k1()) {
        const Ledger ledger(store);
        OUTCOME_TRY(has, ledger.Has(address));
        if (has) {
          return true;
        }
      }
      return prev(address);
    };
    api->WalletImport = {
        [=, prev = api->WalletImport](auto &info) -> outcome::result<Address> {
          if (info.type == SignatureType::kSecp256k1_ledger) {
            OUTCOME_TRY(j_file, codec::json::parse(info.private_key));
            OUTCOME_TRY(ledger_key_info, decode<LedgerKeyInfo>(j_file));

            const Ledger ledger(store);
            OUTCOME_TRY(address, ledger.ImportKey(ledger_key_info));
            return std::move(address);
          }
          return prev(info);
        }};
    api->WalletNew = {
        [=, prev = api->WalletNew](auto &type) -> outcome::result<Address> {
          if (type == "secp256k1-ledger") {
            const Ledger ledger(store);
            OUTCOME_TRY(address, ledger.New());
            return std::move(address);
          } else {
            return prev(type);
          }
        }};
    api->WalletSign = [=, prev = api->WalletSign](
                          auto address,
                          auto data) -> outcome::result<Signature> {
      if (address.isSecp256k1()) {
        const Ledger ledger(store);
        OUTCOME_TRY(has, ledger.Has(address));
        if (has) {
          return ledger.Sign(address, data);
        }
      }
      return prev(address, data);
    };
  }

}  // namespace fc::api
