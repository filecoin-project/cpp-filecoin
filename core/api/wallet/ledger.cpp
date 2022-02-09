/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/wallet/ledger.hpp"

#include <algorithm>
#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "common/bytes_cow.hpp"
#include "common/error_text.hpp"
#include "cpp-ledger/filecoin/ledger_filecoin.hpp"
#include "primitives/address/address_codec.hpp"

namespace fc::api {
  using ledger::kPathLength;
  using ledger::filecoin::LedgerFilecoinManager;
  using primitives::address::decodeFromString;

  const auto app_error = ERROR_TEXT("cannot open Ledger Filecoin App");

  Ledger::Ledger(const MapPtr &store) : store(store) {}

  outcome::result<bool> Ledger::Has(const Address &address) const {
    if (store->contains(encode(address))) {
      OUTCOME_TRY(value, store->get(encode(address)));
      OUTCOME_TRY(j_file, codec::json::parse(value));
      OUTCOME_TRY(ledger_key_info, decode<LedgerKeyInfo>(j_file));

      auto [app, err] = LedgerFilecoinManager::FindLedgerFilecoinApp();
      if (err != std::nullopt) {
        return app_error;
      }

      std::string addr;
      std::tie(std::ignore, std::ignore, addr, err) =
          app->GetAddressPubKeySECP256K1(ledger_key_info.path);
      if (err != std::nullopt) {
        return false;
      }

      OUTCOME_TRY(address_ledger, decodeFromString(addr));
      if (address_ledger != address) {
        return false;
      }

      return true;
    }
    return false;
  }

  outcome::result<Signature> Ledger::Sign(const Address &address,
                                          const Bytes &data) const {
    OUTCOME_TRY(value, store->get(encode(address)));
    OUTCOME_TRY(j_file, codec::json::parse(value));
    OUTCOME_TRY(ledger_key_info, decode<LedgerKeyInfo>(j_file));

    const auto [app, err1] = LedgerFilecoinManager::FindLedgerFilecoinApp();
    if (err1 != std::nullopt) {
      return app_error;
    }

    const auto [sign, err2] = app->SignSECP256K1(ledger_key_info.path, data);
    if (err2 != std::nullopt) {
      return ERROR_TEXT("sign error");
    }

    OUTCOME_TRY(signature,
                Blob<crypto::secp256k1::kSignatureLength>::fromSpan(
                    sign.SignatureBytes()));

    return signature;
  }

  outcome::result<Address> Ledger::ImportKey(
      const LedgerKeyInfo &key_info) const {
    if (key_info.path.size() != kPathLength) {
      return ERROR_TEXT("bad hd path len");
    }

    auto [app, err] = LedgerFilecoinManager::FindLedgerFilecoinApp();
    if (err != std::nullopt) {
      return app_error;
    }

    std::string addr;
    std::tie(std::ignore, std::ignore, addr, err) =
        app->GetAddressPubKeySECP256K1(key_info.path);
    if (err != std::nullopt) {
      return ERROR_TEXT("Ledger does not contain path");
    }

    OUTCOME_TRY(address, decodeFromString(addr));
    if (address != key_info.address) {
      return ERROR_TEXT("wrong address");
    }

    OUTCOME_TRY(bytes, codec::json::format(encode(key_info)));
    OUTCOME_TRY(store->put(encode(key_info.address), std::move(bytes)));

    return key_info.address;
  }

  outcome::result<Address> Ledger::New() const {
    int max_id = -1;

    if (auto it{store->cursor()}) {
      for (it->seekToFirst(); it->isValid(); it->next()) {
        OUTCOME_TRY(j_file, codec::json::parse(it->value()));
        OUTCOME_TRY(key_info, decode<LedgerKeyInfo>(j_file));

        if (key_info.path.size() != kPathLength) {
          return ERROR_TEXT("bad hd path len in store");
        }

        max_id = std::max(max_id, int(key_info.path.back()));
      }
    }

    uint32_t id = max_id + 1;  // new id

    auto [app, err] = LedgerFilecoinManager::FindLedgerFilecoinApp();
    if (err != std::nullopt) {
      return app_error;
    }

    constexpr uint32_t hdHard = 0x80000000;

    // Filecoin base path + new id
    const std::vector<uint32_t> path{hdHard | 44, hdHard | 461, hdHard, 0, id};

    std::string addr;

    std::tie(std::ignore, std::ignore, addr, err) =
        app->GetAddressPubKeySECP256K1(path);
    if (err != std::nullopt) {
      return ERROR_TEXT("getting public key from ledger error");
    }

    std::tie(std::ignore, std::ignore, addr, err) =
        app->ShowAddressPubKeySECP256K1(path);
    if (err != std::nullopt) {
      return ERROR_TEXT("verifying public key with ledger error");
    }

    OUTCOME_TRY(address, decodeFromString(addr));

    app->Close();
    return ImportKey(LedgerKeyInfo{.address = address, .path = path});
  }

}  // namespace fc::api
