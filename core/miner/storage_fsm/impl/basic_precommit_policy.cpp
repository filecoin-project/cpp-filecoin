/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/basic_precommit_policy.hpp"

namespace fc::mining {
  using vm::actor::builtin::v0::miner::kWPoStProvingPeriod;

  ChainEpoch BasicPreCommitPolicy::expiration(
      gsl::span<const types::Piece> pieces) {
    auto maybe_head = api_->ChainHead();
    if (maybe_head.has_error()) {
      logger_->error("Api error: {}", maybe_head.error().message());
      return 0;
    }

    ChainEpoch epoch = maybe_head.value()->height();

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

    *end += kWPoStProvingPeriod - (*end % kWPoStProvingPeriod)
            + proving_boundary_ - 1;

    return *end;
  }

  BasicPreCommitPolicy::BasicPreCommitPolicy(std::shared_ptr<Api> api,
                                             ChainEpoch proving_boundary,
                                             ChainEpoch duration)
      : api_(std::move(api)),
        proving_boundary_(proving_boundary),
        duration_(duration),
        logger_(common::createLogger("basic pre commit policy")) {}
}  // namespace fc::mining
