/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/stored_ask.hpp"

namespace fc::markets::storage::provider {

  // Key to store last ask in datastore
  const Buffer kBestAskKey{codec::cbor::encode("latest-ask").value()};

  StoredAsk::StoredAsk(std::shared_ptr<Datastore> datastore,
                       std::shared_ptr<Api> api,
                       const Address &actor_address)
      : datastore_{std::move(datastore)},
        api_{std::move(api)},
        actor_{actor_address} {}

  outcome::result<void> StoredAsk::addAsk(const TokenAmount &price,
                                          ChainEpoch duration) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    ChainEpoch timestamp = chain_head->height();
    ChainEpoch expiry = chain_head->height() + duration;
    StorageAsk ask{.price = price,
                   .verified_price = price,
                   .min_piece_size = kDefaultMinPieceSize,
                   .max_piece_size = kDefaultMaxPieceSize,
                   .miner = actor_,
                   .timestamp = timestamp,
                   .expiry = expiry,
                   .seq_no = last_signed_storage_ask_
                                 ? last_signed_storage_ask_->ask.seq_no + 1
                                 : 0};
    OUTCOME_TRY(signed_ask, signAsk(ask, *chain_head));
    OUTCOME_TRY(saveSignedAsk(signed_ask));
    return outcome::success();
  }

  outcome::result<SignedStorageAsk> StoredAsk::getAsk(const Address &address) {
    if (address != actor_) {
      return StoredAskError::kWrongAddress;
    }
    if (!last_signed_storage_ask_) {
      OUTCOME_TRY(ask, loadSignedAsk());
      last_signed_storage_ask_ = ask;
    }
    return last_signed_storage_ask_.value();
  }

  outcome::result<SignedStorageAsk> StoredAsk::loadSignedAsk() {
    if (datastore_->contains(kBestAskKey)) {
      OUTCOME_TRY(ask_bytes, datastore_->get(kBestAskKey));
      OUTCOME_TRY(ask,
                  codec::cbor::decode<SignedStorageAsk>(ask_bytes.toVector()));
      return std::move(ask);
    }

    // otherwise return default which 'not actively accepting deals'
    OUTCOME_TRY(chain_head, api_->ChainHead());
    ChainEpoch timestamp = chain_head->height();
    ChainEpoch expiry = chain_head->height() + kDefaultDuration;
    StorageAsk default_ask{.price = kDefaultPrice,
                           .verified_price = kDefaultPrice,
                           .min_piece_size = kDefaultMinPieceSize,
                           .max_piece_size = kDefaultMaxPieceSize,
                           .miner = actor_,
                           .timestamp = timestamp,
                           .expiry = expiry,
                           .seq_no = 0};
    OUTCOME_TRY(signed_ask, signAsk(default_ask, *chain_head));
    return std::move(signed_ask);
  }

  outcome::result<void> StoredAsk::saveSignedAsk(const SignedStorageAsk &ask) {
    OUTCOME_TRY(cbored_ask, codec::cbor::encode(ask));
    OUTCOME_TRY(datastore_->put(kBestAskKey, cbored_ask));
    last_signed_storage_ask_ = ask;
    return outcome::success();
  }

  outcome::result<SignedStorageAsk> StoredAsk::signAsk(
      const StorageAsk &ask, const Tipset &chain_head) {
    OUTCOME_TRY(minfo, api_->StateMinerInfo(actor_, {}));
    OUTCOME_TRY(key_address,
                api_->StateAccountKey(minfo.worker, chain_head.key));
    OUTCOME_TRY(ask_bytes, codec::cbor::encode(ask));
    OUTCOME_TRY(signature, api_->WalletSign(key_address, ask_bytes));
    return SignedStorageAsk{.ask = ask, .signature = signature};
  }

}  // namespace fc::markets::storage::provider

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::provider, StoredAskError, e) {
  using E = fc::markets::storage::provider::StoredAskError;
  switch (e) {
    case E::kWrongAddress:
      return "StoredAskError: wrong address";
    default:
      return "StoredAskError: unknown error";
  }
}
