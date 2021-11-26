/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/stored_ask.hpp"

namespace fc::markets::storage::provider {

  // Key to store last ask in datastore
  const Bytes kBestAskKey{codec::cbor::encode("latest-ask").value()};

  StoredAsk::StoredAsk(std::shared_ptr<Datastore> datastore,
                       std::shared_ptr<FullNodeApi> api,
                       Address actor_address)
      : datastore_{std::move(datastore)},
        api_{std::move(api)},
        actor_{std::move(actor_address)} {}

  outcome::result<void> StoredAsk::addAsk(StorageAsk ask, ChainEpoch duration) {
    assert(ask.miner == actor_);
    OUTCOME_TRY(chain_head, api_->ChainHead());
    ask.timestamp = chain_head->height();
    ask.expiry = ask.timestamp + duration;
    ask.seq_no =
        last_signed_storage_ask_ ? last_signed_storage_ask_->ask.seq_no + 1 : 0;
    OUTCOME_TRY(signed_ask, signAsk(ask, *chain_head));
    OUTCOME_TRY(saveSignedAsk(signed_ask));
    return outcome::success();
  }

  outcome::result<void> StoredAsk::addAsk(const TokenAmount &price,
                                          ChainEpoch duration) {
    return addAsk(
        {
            .price = price,
            .verified_price = price,
            .min_piece_size = kDefaultMinPieceSize,
            .max_piece_size = kDefaultMaxPieceSize,
            .miner = actor_,
        },
        duration);
  }

  outcome::result<SignedStorageAskV1_1_0> StoredAsk::getAsk(
      const Address &address) {
    if (address != actor_) {
      return StoredAskError::kWrongAddress;
    }
    if (!last_signed_storage_ask_) {
      OUTCOME_TRY(ask, loadSignedAsk());
      last_signed_storage_ask_ = ask;
    }
    return last_signed_storage_ask_.value();
  }

  outcome::result<SignedStorageAskV1_1_0> StoredAsk::loadSignedAsk() {
    if (datastore_->contains(kBestAskKey)) {
      OUTCOME_TRY(ask_bytes, datastore_->get(kBestAskKey));
      OUTCOME_TRY(ask, codec::cbor::decode<SignedStorageAskV1_1_0>(ask_bytes));
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

  outcome::result<void> StoredAsk::saveSignedAsk(
      const SignedStorageAskV1_1_0 &ask) {
    OUTCOME_TRY(cbored_ask, codec::cbor::encode(ask));
    OUTCOME_TRY(datastore_->put(kBestAskKey, std::move(cbored_ask)));
    last_signed_storage_ask_ = ask;
    return outcome::success();
  }

  outcome::result<SignedStorageAskV1_1_0> StoredAsk::signAsk(
      const StorageAsk &ask, const Tipset &chain_head) {
    OUTCOME_TRY(minfo, api_->StateMinerInfo(actor_, {}));
    OUTCOME_TRY(key_address,
                api_->StateAccountKey(minfo.worker, chain_head.key));
    SignedStorageAskV1_1_0 signed_ask(ask);
    OUTCOME_TRY(digest, signed_ask.getDigest());
    OUTCOME_TRYA(signed_ask.signature, api_->WalletSign(key_address, digest));
    return signed_ask;
  }

}  // namespace fc::markets::storage::provider

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::provider, StoredAskError, e) {
  using E = fc::markets::storage::provider::StoredAskError;
  if (e == E::kWrongAddress) {
    return "StoredAskError: wrong address";
  }
  return "StoredAskError: unknown error";
}
