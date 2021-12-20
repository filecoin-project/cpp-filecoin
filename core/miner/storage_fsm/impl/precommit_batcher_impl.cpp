/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/precommit_batcher_impl.hpp"

#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/types/miner/v6/monies.hpp"
#include "vm/actor/builtin/v5/miner/miner_actor.hpp"

namespace fc::mining {
  using api::kPushNoSpec;
  using primitives::ChainEpoch;
  using vm::actor::MethodParams;
  using vm::actor::builtin::types::miner::kChainFinality;
  using vm::actor::builtin::v5::miner::PreCommitBatch;
  using vm::actor::builtin::v6::miner::aggregatePreCommitNetworkFee;

  PreCommitBatcherImpl::PreCommitEntry::PreCommitEntry(
      const TokenAmount &number, const SectorPreCommitInfo &info)
      : deposit(number), precommit_info(info){};

  PreCommitBatcherImpl::PreCommitBatcherImpl(
      const std::chrono::milliseconds &max_time,
      std::shared_ptr<FullNodeApi> api,
      const Address &miner_address,
      const std::shared_ptr<Scheduler> &scheduler,
      const AddressSelector &address_selector,
      std::shared_ptr<FeeConfig> fee_config)
      : max_delay_(max_time),
        api_(std::move(api)),
        miner_address_(miner_address),
        scheduler_(scheduler),
        closest_cutoff_(max_time),
        fee_config_(std::move(fee_config)),
        address_selector_(address_selector) {
    cutoff_start_ = std::chrono::system_clock::now();
    logger_ = common::createLogger("batcher");
    logger_->info("Batcher has been started");
    reschedule(max_delay_);
  }

  void PreCommitBatcherImpl::reschedule(std::chrono::milliseconds time) {
    handle_ = scheduler_->scheduleWithHandle(
        [&]() {
          std::unique_lock<std::mutex> locker(mutex_);
          const auto maybe_result = sendBatch();
          for (const auto &[key, cb] : callbacks_) {
            cb(maybe_result);
          }
          callbacks_.clear();
          cutoff_start_ = std::chrono::system_clock::now();
          closest_cutoff_ = max_delay_;

          // reschedule during scheduler callback, will not throw
          handle_.reschedule(max_delay_).value();
        },
        time);
  }

  outcome::result<CID> PreCommitBatcherImpl::sendBatch() {
    if (batch_storage_.size() != 0) {
      logger_->info("Sending procedure started");
      OUTCOME_TRY(head, api_->ChainHead());
      OUTCOME_TRY(minfo, api_->StateMinerInfo(miner_address_, head->key));

      PreCommitBatch::Params params;
      TokenAmount mutual_deposit;
      for (const auto &data : batch_storage_) {
        mutual_deposit += data.second.deposit;
        params.sectors.push_back(data.second.precommit_info);
      }
      TokenAmount max_fee =
          fee_config_->max_precommit_batch_gas_fee.FeeForSector(
              params.sectors.size());

      OUTCOME_TRY(tipset, api_->ChainGetTipSet(head->key));
      auto base_fee = tipset->blks[0].parent_base_fee;
      TokenAmount agg_fee_raw =
          aggregatePreCommitNetworkFee(params.sectors.size(), base_fee);

      static TokenAmount kAggFeeNum{110};
      static TokenAmount kAggFeeDen{100};
      TokenAmount agg_fee = bigdiv(agg_fee_raw * kAggFeeNum, kAggFeeDen);

      auto need_funds = mutual_deposit + agg_fee;

      // TODO: Collateral Send Amount

      TokenAmount good_funds = max_fee + need_funds;
      OUTCOME_TRY(encodedParams, codec::cbor::encode(params));
      OUTCOME_TRY(address, address_selector_(minfo, good_funds, api_));
      OUTCOME_TRY(signed_message,
                  api_->MpoolPushMessage(
                      vm::message::UnsignedMessage(miner_address_,
                                                   address,
                                                   0,
                                                   need_funds,
                                                   max_fee,
                                                   {},
                                                   PreCommitBatch::Number,
                                                   MethodParams{encodedParams}),
                      kPushNoSpec));

      batch_storage_.clear();
      logger_->info("Sending procedure completed");
      cutoff_start_ = std::chrono::system_clock::now();
      return signed_message.getCid();
    }
    cutoff_start_ = std::chrono::system_clock::now();
    return ERROR_TEXT("Empty Batcher");
  }

  void PreCommitBatcherImpl::forceSend() {
    std::unique_lock<std::mutex> locker(mutex_);
    forceSendWithoutLock();
  }

  void PreCommitBatcherImpl::forceSendWithoutLock() {
    const auto maybe_result = sendBatch();
    for (const auto &[key, cb] : callbacks_) {
      cb(maybe_result);
    }
    callbacks_.clear();
    cutoff_start_ = std::chrono::system_clock::now();
    closest_cutoff_ = max_delay_;
    reschedule(max_delay_);
  }

  void PreCommitBatcherImpl::setPreCommitCutoff(const ChainEpoch &current_epoch,
                                                const SectorInfo &sector_info) {
    ChainEpoch cutoff_epoch =
        sector_info.ticket_epoch
        + static_cast<int64_t>(kEpochsInDay + kChainFinality);
    ChainEpoch start_epoch{};
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
      forceSendWithoutLock();
    } else {
      const auto temp_cutoff = std::chrono::milliseconds(
          (cutoff_epoch - current_epoch) * kEpochDurationSeconds);
      if ((closest_cutoff_
               - std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now() - cutoff_start_)
           > temp_cutoff)) {
        cutoff_start_ = std::chrono::system_clock::now();
        reschedule(temp_cutoff);
        closest_cutoff_ = temp_cutoff;
      }
    }
  }

  outcome::result<void> PreCommitBatcherImpl::addPreCommit(
      const SectorInfo &sector_info,
      const TokenAmount &deposit,
      const api::SectorPreCommitInfo &precommit_info,
      const PrecommitCallback &callback) {
    std::unique_lock<std::mutex> locker(mutex_);
    OUTCOME_TRY(head, api_->ChainHead());

    const auto &sn = sector_info.sector_number;
    batch_storage_[sn] = PreCommitEntry(deposit, precommit_info);
    callbacks_[sn] = callback;  // TODO: batcher upper limit
    setPreCommitCutoff(head->epoch(), sector_info);
    return outcome::success();
  }
}  // namespace fc::mining
