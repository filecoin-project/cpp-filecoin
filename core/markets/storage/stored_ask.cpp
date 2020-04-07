/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/stored_ask.hpp"

namespace fc::markets::storage {

  outcome::result<void> StoredAsk::addAsk(const TokenAmount &price,
                                          ChainEpoch duration) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t sequential_number{0};
    if (signed_storage_ask_) {
      sequential_number = signed_storage_ask_->ask.seq_no + 1;
    }

    OUTCOME_TRY(chain_head, storage_provider_node_->getChainHead());
    auto &&tipset_token{chain_head.first};
    auto &&height{chain_head.second};
    StorageAsk ask;
    ask.price = price;
    ask.timestamp = height;
    ask.expiry = height + duration;
    ask.miner = actor_;
    ask.seq_no = sequential_number;
    ask.min_piece_size = kDefaultMinPieceSize;

    // TODO signature

    return outcome::success();
  }
}  // namespace fc::markets::storage
