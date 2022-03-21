/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/basic_precommit_policy.hpp"

namespace fc::mining {
  using vm::actor::builtin::types::miner::kMaxSectorExpirationExtension;
  using vm::actor::builtin::types::miner::kMinSectorExpiration;
  using vm::actor::builtin::types::miner::kWPoStProvingPeriod;

  outcome::result<ChainEpoch> BasicPreCommitPolicy::expiration(
      gsl::span<const types::Piece> pieces) {
    OUTCOME_TRY(head, api_->ChainHead());

    const ChainEpoch epoch = head->height();

    boost::optional<ChainEpoch> end = boost::none;

    for (const auto &piece : pieces) {
      if (!piece.deal_info.has_value()) {
        continue;
      }

      if (piece.deal_info->deal_schedule.end_epoch < ChainEpoch(epoch)) {
        logger_->warn("piece schedule {} ended before current epoch {}",
                      piece.piece.cid.toString().value(),
                      epoch);
        continue;
      }

      if (!end.has_value()
          || end.get() < piece.deal_info->deal_schedule.end_epoch) {
        end = piece.deal_info->deal_schedule.end_epoch;
      }
    }

    if (!end.has_value()) {
      end = epoch + duration_;
    }

    const auto min_exp = epoch + kMinSectorExpiration + kWPoStProvingPeriod;

    if (end < min_exp) {
      end = min_exp;
    }

    return *end;
  }

  BasicPreCommitPolicy::BasicPreCommitPolicy(
      std::shared_ptr<FullNodeApi> api,
      ChainEpoch proving_boundary,
      std::chrono::seconds sector_lifetime)
      : api_(std::move(api)),
        proving_boundary_(proving_boundary),
        logger_(common::createLogger("basic pre commit policy")) {
    ChainEpoch sector_lifetime_ =
        std::ceil(sector_lifetime.count() / double(kBlockDelaySecs));
    if (sector_lifetime_ == 0) {
      sector_lifetime_ = kMaxSectorExpirationExtension;
    }

    if (sector_lifetime_ < kMinSectorExpiration) {
      duration_ = kMinSectorExpiration;
    } else if (sector_lifetime_ > kMaxSectorExpirationExtension) {
      duration_ = kMaxSectorExpirationExtension;
    } else {
      duration_ = sector_lifetime_ - proving_boundary_;
    }
  }
}  // namespace fc::mining
