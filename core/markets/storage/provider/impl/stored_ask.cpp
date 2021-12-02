/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/stored_ask.hpp"

namespace fc::markets::storage::provider {

  // Key to store last ask in datastore
  const Bytes kBestAskKey{codec::cbor::encode("latest-ask").value()};

  outcome::result<std::shared_ptr<StoredAsk>> StoredAsk::newStoredAsk(
      std::shared_ptr<Datastore> datastore,
      std::shared_ptr<FullNodeApi> api,
      Address actor_address) {
    struct make_unique_enabler : StoredAsk {
      make_unique_enabler(std::shared_ptr<Datastore> datastore,
                          std::shared_ptr<FullNodeApi> api,
                          Address actor_address)
          : StoredAsk(
              std::move(datastore), std::move(api), std::move(actor_address)){};
    };

    std::unique_ptr<StoredAsk> stored_ask =
        std::make_unique<make_unique_enabler>(
            std::move(datastore), std::move(api), std::move(actor_address));

    OUTCOME_TRY(maybe_ask, stored_ask->tryLoadSignedAsk());

    if (not maybe_ask.has_value()) {
      OUTCOME_TRY(stored_ask->addAsk(kDefaultPrice, kDefaultDuration));
    }

    return std::move(stored_ask);
  }

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
    auto min_size = kDefaultMinPieceSize;
    auto max_size = kDefaultMaxPieceSize;

    if (last_signed_storage_ask_.has_value()) {
      auto &ask{(*last_signed_storage_ask_).ask};
      min_size = ask.min_piece_size;
      max_size = ask.max_piece_size;
    }

    return addAsk(
        {
            .price = price,
            .verified_price = price,
            .min_piece_size = min_size,
            .max_piece_size = max_size,
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
    OUTCOME_TRY(ask_bytes, datastore_->get(kBestAskKey));
    OUTCOME_TRY(ask, codec::cbor::decode<SignedStorageAskV1_1_0>(ask_bytes));
    return std::move(ask);
  }

  outcome::result<boost::optional<SignedStorageAskV1_1_0>>
  StoredAsk::tryLoadSignedAsk() {
    if (not datastore_->contains(kBestAskKey)) {
      return boost::none;
    }
    OUTCOME_TRY(ask, loadSignedAsk());
    return std::move(ask);
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
    return std::move(signed_ask);
  }

}  // namespace fc::markets::storage::provider

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::provider, StoredAskError, e) {
  using E = fc::markets::storage::provider::StoredAskError;
  if (e == E::kWrongAddress) {
    return "StoredAskError: wrong address";
  }
  return "StoredAskError: unknown error";
}
