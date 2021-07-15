/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/precommit_batcher_impl.hpp"

#include <utility>
#include "const.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::mining {
  using api::kPushNoSpec;
  using fc::vm::actor::builtin::v0::miner::PreCommitBatch;
  using libp2p::protocol::scheduler::toTicks;
  using primitives::ChainEpoch;
  using vm::actor::MethodParams;
  using vm::actor::builtin::types::miner::kChainFinality;
  PreCommitBatcherImpl::PreCommitBatcherImpl(const Ticks &max_time,
                                             std::shared_ptr<FullNodeApi> api,
                                             const Address &miner_address,
                                             const Ticks &closest_cutoff)
      : max_delay_(max_time),
        api_(std::move(api)),
        miner_address_(miner_address),
        closest_cutoff_(closest_cutoff) {}

  PreCommitBatcherImpl::PreCommitEntry::PreCommitEntry(
      const TokenAmount &number, const SectorPreCommitInfo &info)
      : deposit(number), precommit_info(info){};

  outcome::result<std::shared_ptr<PreCommitBatcherImpl>>
  PreCommitBatcherImpl::makeBatcher(
      const Ticks &max_wait,
      std::shared_ptr<FullNodeApi> api,
      const std::shared_ptr<libp2p::protocol::Scheduler>& scheduler,
      const Address &miner_address) {
    std::shared_ptr<PreCommitBatcherImpl> batcher =
        std::make_shared<PreCommitBatcherImpl>(
            max_wait, std::move(api), miner_address, max_wait);

    batcher->cutoff_start_ = std::chrono::system_clock::now();
    batcher->logger_ = common::createLogger("batcher");
    batcher->logger_->info("Bather have been started");
    batcher->handle_ = scheduler->schedule(max_wait, [=]() {
      auto maybe_result = batcher->sendBatch();
      for (const auto &[key, cb] : batcher->callbacks_) {
        cb(maybe_result);
      }
      batcher->callbacks_.clear();
      batcher->handle_.reschedule(max_wait);
    });
    return batcher;
  }

  outcome::result<CID> PreCommitBatcherImpl::sendBatch() {
    std::unique_lock<std::mutex> locker(mutex_, std::defer_lock);
    // TODO(Elestrias): [FIL-398] goodFunds = mutualDeposit + MaxFee; -  for
    // checking payable
    CID message_cid;
    if (batch_storage_.size() != 0) {
      logger_->info("Sending procedure started");

      OUTCOME_TRY(head, api_->ChainHead());
      OUTCOME_TRY(minfo, api_->StateMinerInfo(miner_address_, head->key));

      PreCommitBatch::Params params = {};

      for (const auto &data : batch_storage_) {
        mutual_deposit_ += data.second.deposit;
        params.sectors.push_back(data.second.precommit_info);
      }

      OUTCOME_TRY(encodedParams, codec::cbor::encode(params));

      OUTCOME_TRY(signed_message,
                  api_->MpoolPushMessage(
                      vm::message::UnsignedMessage(
                          miner_address_,
                          minfo.worker,  // TODO: handle worker
                          0,
                          mutual_deposit_,
                          {},
                          {},
                          25,  // TODO (m.tagirov) Miner actor v5 PreCommitBatch
                          MethodParams{encodedParams}),
                      kPushNoSpec));

      mutual_deposit_ = 0;
      batch_storage_.clear();
      message_cid = signed_message.getCid();
      logger_->info("Sending procedure completed");
      cutoff_start_ = std::chrono::system_clock::now();
      return message_cid;
    }
    cutoff_start_ = std::chrono::system_clock::now();
    return ERROR_TEXT("Empty Batcher");
  }

  void PreCommitBatcherImpl::forceSend() {
    auto maybe_result = sendBatch();
    for (const auto &[key, cb] : callbacks_) {
      cb(maybe_result);
    }
    callbacks_.clear();
    handle_.reschedule(max_delay_);
  }

  void PreCommitBatcherImpl::setPreCommitCutoff(const ChainEpoch &current_epoch,
                                                const SectorInfo &sector_info) {
    ChainEpoch cutoff_epoch =
        sector_info.ticket_epoch
        + static_cast<int64_t>(fc::kEpochsInDay + kChainFinality);
    ChainEpoch start_epoch{};
    for (auto p : sector_info.pieces) {
      if (!p.deal_info) {
        continue;
      }
      start_epoch = p.deal_info->deal_schedule.start_epoch;
      if (start_epoch < cutoff_epoch) {
        cutoff_epoch = start_epoch;
      }
    }
    if (cutoff_epoch <= current_epoch) {
      forceSend();
    } else {
      const Ticks temp_cutoff = toTicks(std::chrono::seconds(
          (cutoff_epoch - current_epoch) * kEpochDurationSeconds));
      if ((closest_cutoff_
               - toTicks(std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now() - cutoff_start_))
           > temp_cutoff)) {
        cutoff_start_ = std::chrono::system_clock::now();
        handle_.reschedule(temp_cutoff);
        closest_cutoff_ = temp_cutoff;
      }
    }
  }

  outcome::result<void> PreCommitBatcherImpl::addPreCommit(
      const SectorInfo &sector_info,
      const TokenAmount &deposit,
      const api::SectorPreCommitInfo &precommit_info,
      const PrecommitCallback &callback) {
    std::unique_lock<std::mutex> locker(mutex_, std::defer_lock);
    OUTCOME_TRY(head, api_->ChainHead());

    const auto &sn = sector_info.sector_number;
    batch_storage_[sn] = PreCommitEntry(deposit, precommit_info);
    callbacks_[sn] = callback;  // TODO: batcher upper limit
    setPreCommitCutoff(head->epoch(), sector_info);
    return outcome::success();
  }
}  // namespace fc::mining
