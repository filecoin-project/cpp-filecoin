/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_SYNCER_STATE_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_SYNCER_STATE_HPP

#include "clock/impl/utc_clock_impl.hpp"
#include "clock/time.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::blockchain {

  enum class SyncStateStage : int {
    StageIdle,
    StageHeaders,
    StagePersistHeaders,
    StageMessages,
    StageSyncComplete,
    StageSyncErrored
  };

  /** @brief makes human-readable description of sync state stage */
  std::string toString(SyncStateStage value);

  struct SyncerState {
    using Tipset = primitives::tipset::Tipset;

    // not sure about references
    void initialize(Tipset b, Tipset t);

    void setStage(SyncStateStage stage);

    void setError(outcome::result<void> error);

    SyncerState takeSnapshot() const;

    boost::optional<Tipset> target;
    boost::optional<Tipset> base;
    SyncStateStage stage{SyncStateStage::StageHeaders};
    uint64_t height{0u};
    outcome::result<void> error{outcome::success()};
    boost::optional<clock::Time> start{boost::none};
    boost::optional<clock::Time> end{boost::none};
  };

}  // namespace fc::blockchain

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_SYNCER_STATE_HPP
