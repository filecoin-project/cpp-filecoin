/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE__STORED_ASK_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE__STORED_ASK_HPP

#include <memory>
#include <mutex>

#include <boost/optional.hpp>
#include "api/api.hpp"
#include "common/outcome.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"
#include "provider.hpp"
#include "storage/face/persistent_map.hpp"

namespace fc::markets::storage::provider {
  using api::Api;
  using common::Buffer;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;
  using primitives::tipset::Tipset;

  using Datastore = fc::storage::face::PersistentMap<Buffer, Buffer>;

  static const TokenAmount kDefaultPrice = 500'000'000;
  static constexpr ChainEpoch kDefaultDuration = 1'000'000;
  static const PaddedPieceSize kDefaultMinPieceSize(256);
  static const PaddedPieceSize kDefaultMaxPieceSize{1 << 20};

  /**
   * Storage for storage market asks.
   */
  class StoredAsk {
   public:
    StoredAsk(std::shared_ptr<Datastore> datastore,
              std::shared_ptr<Api> api,
              const Address &actor_address);

    auto addAsk(const TokenAmount &price, ChainEpoch duration)
        -> outcome::result<void>;

    auto getAsk(const Address &address) -> outcome::result<SignedStorageAsk>;

   private:
    /**
     * Loads last storage ask or creates default one
     */
    auto loadSignedAsk() -> outcome::result<SignedStorageAsk>;

    /**
     * Saves last storage ask to datastore
     * @param ask to save
     */
    auto saveSignedAsk(const SignedStorageAsk &ask) -> outcome::result<void>;

    auto signAsk(const StorageAsk &ask, const Tipset &chain_head)
        -> outcome::result<SignedStorageAsk>;

    boost::optional<SignedStorageAsk> last_signed_storage_ask_;
    std::shared_ptr<Datastore> datastore_;
    std::shared_ptr<Api> api_;
    Address actor_;
  };

  enum class StoredAskError { kWrongAddress = 1 };

}  // namespace fc::markets::storage::provider

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::provider, StoredAskError);

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE__STORED_ASK_HPP
