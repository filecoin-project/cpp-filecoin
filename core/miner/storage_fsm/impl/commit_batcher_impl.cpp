/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "commit_batcher_impl.hpp"
#include <iterator>
#include "vm/actor/builtin/v5/miner/miner_actor.hpp"

namespace fc::mining {
  using fc::primitives::ActorId;
  using primitives::BigInt;
  using primitives::sector::AggregateSealVerifyInfo;
  using vm::actor::builtin::types::miner::kChainFinality;
  using vm::actor::builtin::types::miner::SectorPreCommitOnChainInfo;
  using vm::actor::builtin::v5::miner::ProveCommitAggregate;

  CommitBatcherImpl::CommitBatcherImpl(
      const std::chrono::milliseconds &max_time,
      const size_t &max_size_callback,
      const std::shared_ptr<Scheduler> &scheduler)
      : max_delay_(max_time), max_size_callback_(max_size_callback) {
    cutoff_start_ = std::chrono::system_clock::now();
    handle_ =
        scheduler_->scheduleWithHandle([&]() { sendCallbacks(); }, max_delay_);
  }

  outcome::result<void> CommitBatcherImpl::addCommit(
      const SectorInfo &sector_info,
      const AggregateInput &aggregate_input,
      const CommitCallback &callback) {
    std::unique_lock<std::mutex> locker(mutex_storage_);

    const SectorNumber &sector_number = sector_info.sector_number;
    OUTCOME_TRY(head, api_->ChainHead());

    union_storage_[sector_number] = PairStorage{aggregate_input, callback};

    if (union_storage_.size() >= max_size_callback_) {
      sendCallbacks();
    }
    // Вынести мьютексы
    setCommitCutoff(head->epoch(), sector_info);

    return outcome::success();
  }

  void CommitBatcherImpl::forceSend() {}

  void CommitBatcherImpl::sendCallbacks() {
    std::unique_lock<std::mutex> locker(mutex_storage_);
    MapPairStorage union_storage_for_send_(
        std::make_move_iterator(union_storage_.begin()),
        std::make_move_iterator(union_storage_.end()));
    locker.unlock();

    const auto maybe_result = sendBatch(union_storage_for_send_);
    for (const auto &[key, pair_storage] : union_storage_for_send_) {
      pair_storage.commit_callback(maybe_result);
    }

    cutoff_start_ = std::chrono::system_clock::now();
    closest_cutoff_ = max_delay_;

    handle_.reschedule(max_delay_).value();
  }

  outcome::result<CID> CommitBatcherImpl::sendBatch(
      MapPairStorage &union_storage_for_send) {
    if (union_storage_for_send.empty()) {
      cutoff_start_ = std::chrono::system_clock::now();
      return ERROR_TEXT("Empty Batcher");
    }
    OUTCOME_TRY(head, api_->ChainHead());

    // TODO ?
    const size_t total = union_storage_for_send.size();

    ProveCommitAggregate::Params params;

    std::vector<std::vector<uint8_t>> proofs;
    proofs.reserve(total);

    BigInt collateral = 0;

    for (const auto &[sector_number, pair_storage] : union_storage_for_send) {
      OUTCOME_TRY(sc, getSectorCollateral(sector_number, head->key));
      collateral = collateral + sc;

      params.sectors.insert(sector_number);
    }

    for (const auto &[sector_number, pair_storage] : union_storage_for_send) {
      proofs.push_back(pair_storage.aggregate_input.proof);
    }

    const ActorId mid = miner_address_.getId();
    // TODO maybe long (AggregateSealProofs)

    // TODO params.proof = proof_->AggregateSealProofs(); // OUTCOME_TRY
    OUTCOME_TRY(a, proof_->AggregateSealProofs());

    auto enc = codec::cbor::encode(params);
    OUTCOME_TRY(mi, api_->StateMinerInfo(miner_address_, head->key));

    // BigDiv usage вместо /(обычное деление)

    const TokenAmount max_fee =
        fee_config_->max_commit_batch_gas_fee.FeeForSector(proofs.size());

    /*
     * API_METHOD(StateMinerInfo,
     * jwt::kReadPermission,
     * MinerInfo,
     * const Address &,
     * const TipsetKey &)
     */
    // OTCOME_TRY(mi, api_->StateMinerInfo());

    // TODO OUTCOME_TRY(bf, api_->ChainBaseFee(head));
    OUTCOME_TRY(nv, api_->StateNetworkVersion(head->key));
  }

  void CommitBatcherImpl::setCommitCutoff(const ChainEpoch &current_epoch,
                                          const SectorInfo &sector_info) {
    ChainEpoch cutoff_epoch =
        sector_info.ticket_epoch
        + static_cast<int64_t>(kEpochsInDay + kChainFinality);
    ChainEpoch start_epoch;
    for (const auto &piece : sector_info.pieces) {
      if (!piece.deal_info) {
        continue;
      }
      start_epoch = piece.deal_info->deal_schedule.start_epoch;
      if (start_epoch < cutoff_epoch) {
        cutoff_epoch = start_epoch;
      }
    }
    if (cutoff_epoch <= current_epoch) {
      forceSend();
    } else {
      const auto temp_cutoff = std::chrono::milliseconds(
          (cutoff_epoch - current_epoch) * kEpochDurationSeconds);
      if ((closest_cutoff_
               - std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now() - cutoff_start_)
           > temp_cutoff)) {
        cutoff_start_ = std::chrono::system_clock::now();
        handle_.reschedule(max_delay_).value();
        closest_cutoff_ = temp_cutoff;
      }
    }
  }

  outcome::result<TokenAmount> CommitBatcherImpl::getSectorCollateral(
      const SectorNumber &sector_number, const TipsetKey &tip_set_key) {
    OUTCOME_TRY(pci,
                api_->StateSectorPreCommitInfo(
                    miner_address_, sector_number, tip_set_key));

    OUTCOME_TRY(collateral,
                api_->StateMinerInitialPledgeCollateral(
                    miner_address_, pci.info, tip_set_key));

    collateral = collateral + pci.precommit_deposit;
    collateral = std::max(BigInt(0), collateral);

    return collateral;
  }

}  // namespace fc::mining
