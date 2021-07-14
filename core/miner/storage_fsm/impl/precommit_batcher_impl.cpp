/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/precommit_batcher_impl.hpp"

#include <utility>
#include "const.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
namespace fc::mining {
  using api::kPushNoSpec;
  using fc::vm::actor::builtin::v0::miner::PreCommitBatch;
  using libp2p::protocol::scheduler::toTicks;
  using primitives::ChainEpoch;
  using vm::actor::builtin::types::miner::kChainFinality;
  PreCommitBatcherImpl::PreCommitBatcherImpl(const Ticks &max_time,
                                             std::shared_ptr<FullNodeApi> api,
                                             const Address &miner_address,
                                             const Ticks &closest_cutoff)
      : max_delay_(max_time),
        api_(std::move(api)),
        miner_address_(miner_address),
        closest_cutoff_(closest_cutoff){};

  PreCommitBatcherImpl::PreCommitEntry::PreCommitEntry(
      TokenAmount number, api::SectorPreCommitInfo info)
      : deposit(std::move(number)), precommit_info(std::move(info)){};

  outcome::result<std::shared_ptr<PreCommitBatcherImpl>>
  PreCommitBatcherImpl::makeBatcher(
      const Ticks &max_wait,
      std::shared_ptr<FullNodeApi> api,
      std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
      const Address &miner_address) {
    std::shared_ptr<PreCommitBatcherImpl> batcher =
        std::make_shared<PreCommitBatcherImpl>(
            max_wait, std::move(api), miner_address, max_wait);

    batcher->cutoff_start_ = std::chrono::system_clock::now();
    batcher->logger_ = common::createLogger("batcher");
    batcher->logger_->info("Bather have been started");
    batcher->handle_ = scheduler->schedule(max_wait, [=]() {
      batcher->sendBatch();
      batcher->handle_.reschedule(max_wait);  // TODO: maybe in gsl::finally
    });
    return batcher;
  }

  void PreCommitBatcherImpl::sendBatch() {
    std::unique_lock<std::mutex> locker(mutex_, std::defer_lock);
    // TODO(Elestrias): [FIL-398] goodFunds = mutualDeposit + MaxFee; -  for
    // checking payable

    if (batch_storage_.size() != 0) {
      logger_->info("Sending procedure started");
      auto head = api_->ChainHead().value();
      auto minfo = api_->StateMinerInfo(miner_address_, head->key).value();

      PreCommitBatch::Params params = {};
      for (const auto &data : batch_storage_) {
        mutual_deposit_ += data.second.deposit;
        params.sectors.push_back(data.second.precommit_info);
      }
      auto encodedParams = codec::cbor::encode(params);
      if (encodedParams.has_error()) {
        logger_->error("Error has occurred during parameters encoding: {}",
                       encodedParams.error().message());
      }
      logger_->info("Sending data to the network...");
      auto maybe_signed = api_->MpoolPushMessage(
          vm::message::UnsignedMessage(
              miner_address_,
              minfo.worker,  // TODO: handle worker
              0,
              mutual_deposit_,
              {},
              {},
              25,  // TODO (m.tagirov) Miner actor v5 PreCommitBatch
              MethodParams{encodedParams.value()}),
          kPushNoSpec);

      if (maybe_signed.has_error()) {
        logger_->error("Error has occurred during batch sending: {}",
                       maybe_signed.error().message());  // TODO: maybe logs
      }

      mutual_deposit_ = 0;
      batch_storage_.clear();
    }
    cutoff_start_ = std::chrono::system_clock::now();
    logger_->info("Sending procedure completed");
  }

  outcome::result<void> PreCommitBatcherImpl::forceSend() {
    sendBatch();
    handle_.reschedule(max_delay_);
    return outcome::success();
  }

  outcome::result<void> PreCommitBatcherImpl::setPreCommitCutoff(
      const ChainEpoch &curEpoch, const SectorInfo &si) {
    ChainEpoch cutoffEpoch =
        si.ticket_epoch
        + static_cast<int64_t>(fc::kEpochsInDay + kChainFinality);
    ChainEpoch startEpoch{};
    for (auto p : si.pieces) {
      if (!p.deal_info) {
        continue;
      }
      startEpoch = p.deal_info->deal_schedule.start_epoch;
      if (startEpoch < cutoffEpoch) {
        cutoffEpoch = startEpoch;
      }
    }
    if (cutoffEpoch <= curEpoch) {
      OUTCOME_TRY(forceSend());
    } else {
      Ticks tempCutoff = toTicks(std::chrono::seconds((cutoffEpoch - curEpoch)
                                                      * kEpochDurationSeconds));
      if ((closest_cutoff_
               - toTicks(std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now() - cutoff_start_))
           > tempCutoff)) {
        cutoff_start_ = std::chrono::system_clock::now();
        handle_.reschedule(tempCutoff);
        closest_cutoff_ = tempCutoff;
      }
    }
    return outcome::success();
  }

  outcome::result<void> PreCommitBatcherImpl::addPreCommit(
      const SectorInfo &secInf,
      const TokenAmount &deposit,
      const api::SectorPreCommitInfo &pcInfo) {
    std::unique_lock<std::mutex> locker(mutex_, std::defer_lock);
    OUTCOME_TRY(head, api_->ChainHead());

    auto sn = secInf.sector_number;
    batch_storage_[sn] = PreCommitEntry(deposit, pcInfo);

    setPreCommitCutoff(head->epoch(), secInf);
    return outcome::success();
  }
}  // namespace fc::mining
