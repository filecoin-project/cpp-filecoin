/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_EVENTS_IMPL_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_EVENTS_IMPL_HPP

#include "miner/storage_fsm/events.hpp"

#include "miner/storage_fsm/tipset_cache.hpp"
#include "storage/chain/chain_store.hpp"

namespace fc::mining {
  using primitives::tipset::HeadChange;
  using primitives::tipset::HeadChangeType;

  class EventsImpl : public Events {
   public:
    outcome::result<void> chainAt(HeightHandler handler,
                                  RevertHandler revert_handler,
                                  ChainEpoch confidence,
                                  ChainEpoch height) override;

   private:
    struct HeightHandle {
      ChainEpoch confidence;
      bool called;

      HeightHandler handler;
      RevertHandler revert;
    };

    void callback_function(const HeadChange &change);

    fc::storage::blockchain::ChainStore::connection_t connection_;

    uint64_t global_id_;

    std::unordered_map<uint64_t, HeightHandle> height_triggers_;

    std::map<ChainEpoch, std::vector<uint64_t>>
        height_to_trigger_;  // for apply
    std::map<ChainEpoch, std::vector<uint64_t>>
        message_height_to_trigger_;  // for revert

    std::mutex mutex_;

    std::shared_ptr<TipsetCache> tipset_cache_;
  };

}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_EVENTS_IMPL_HPP
