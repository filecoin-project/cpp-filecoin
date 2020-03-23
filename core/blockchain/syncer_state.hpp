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
    STAGE_IDLE,
    STAGE_HEADERS,
    STAGE_PERSIST_HEADERS,
    STAGE_MESSAGES,
    STAGE_SYNC_COMPLETE,
    STAGE_SYNC_ERRORED,
  };

  /** @brief makes human-readable description of sync state stage */
  std::string toString(SyncStateStage value);

  /** struct SyncerState is state required for external syncer */
  struct SyncerState {
    using Tipset = primitives::tipset::Tipset;

    /** @brief initializes state */
    void initialize(Tipset b, Tipset t);

    /** @brief set state */
    void setStage(SyncStateStage stage);

    /** @brief set error */
    void setError(outcome::result<void> error);

    /** @brief takes state snapshot */
    SyncerState takeSnapshot() const;

    boost::optional<Tipset> target;
    boost::optional<Tipset> base;
    SyncStateStage stage{SyncStateStage::STAGE_HEADERS};
    uint64_t height{0u};
    outcome::result<void> error{outcome::success()};
    boost::optional<clock::Time> start{boost::none};
    boost::optional<clock::Time> end{boost::none};
  };

}  // namespace fc::blockchain

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_SYNCER_STATE_HPP
