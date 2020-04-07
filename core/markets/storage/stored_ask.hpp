/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE__STORED_ASK_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE__STORED_ASK_HPP

#include <memory>
#include <mutex>

#include <boost/optional.hpp>
#include "common/outcome.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/node_api/storage_provider_node.hpp"
#include "markets/storage/provider.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"
#include "storage/chain/chain_data_store.hpp"

namespace fc::markets::storage {
  using ::fc::storage::blockchain::ChainDataStore;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;

  static const TokenAmount kDefaultPrice = 500'000'000;
  static constexpr ChainEpoch kDefaultDuration = 1'000'000;
  static const PaddedPieceSize kDefaultMinPieceSize(256);

  class StoredAsk {
   public:
    // TODO constructor

    auto addAsk(const TokenAmount &price, ChainEpoch duration)
        -> outcome::result<void>;

    auto getAsk(const Address &address)
        -> outcome::result<boost::optional<std::shared_ptr<SignedStorageAsk>>>;

   private:
    std::mutex mutex_;
    std::shared_ptr<SignedStorageAsk>
        signed_storage_ask_;  // maybe boost::optional
    std::shared_ptr<ChainDataStore> datastore_;
    std::shared_ptr<StorageProviderNode> storage_provider_node_;
    Address actor_;
  };

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE__STORED_ASK_HPP
