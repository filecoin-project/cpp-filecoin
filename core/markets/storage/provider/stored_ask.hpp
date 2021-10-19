/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <mutex>

#include <boost/optional.hpp>
#include "api/full_node/node_api.hpp"
#include "common/outcome.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"
#include "provider.hpp"
#include "storage/face/persistent_map.hpp"

namespace fc::markets::storage::provider {
  using api::FullNodeApi;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;
  using primitives::tipset::Tipset;

  using Datastore = fc::storage::face::PersistentMap<Bytes, Bytes>;

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
              std::shared_ptr<FullNodeApi> api,
              const Address &actor_address);

    auto addAsk(StorageAsk ask, ChainEpoch duration) -> outcome::result<void>;

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
    std::shared_ptr<FullNodeApi> api_;
    Address actor_;
  };

  enum class StoredAskError { kWrongAddress = 1 };

}  // namespace fc::markets::storage::provider

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::provider, StoredAskError);
