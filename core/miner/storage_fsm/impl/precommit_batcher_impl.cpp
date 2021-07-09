/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/precommit_batcher_impl.hpp"

#include <utility>

namespace fc::mining {

  PreCommitBatcherImpl::PreCommitBatcherImpl(size_t maxTime,
                                             std::shared_ptr<FullNodeApi> api)
      : api_(std::move(api)){};

  PreCommitBatcherImpl::preCommitEntry::preCommitEntry(
      primitives::TokenAmount number, api::SectorPreCommitInfo info)
      : deposit(std::move(number)), pci(std::move(info)){};

  PreCommitBatcherImpl::preCommitEntry &
  PreCommitBatcherImpl::preCommitEntry::operator=(const preCommitEntry &x) =
      default;

  outcome::result<std::shared_ptr<PreCommitBatcherImpl>>
  PreCommitBatcherImpl::makeBatcher(
      size_t maxWait,
      std::shared_ptr<FullNodeApi> api,
      std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
      Address &miner_address) {
    struct make_unique_enabler : public PreCommitBatcherImpl {
      make_unique_enabler(size_t MaxTime, std::shared_ptr<FullNodeApi> api)
          : PreCommitBatcherImpl(MaxTime, std::move(api)){};
    };

    std::shared_ptr<PreCommitBatcherImpl> batcher =
        std::make_unique<make_unique_enabler>(maxWait, std::move(api));

    batcher->miner_address_ = miner_address;
    batcher->maxDelay = maxWait;

    batcher -> cutoffStart = std::chrono::system_clock::now();
    batcher -> closestCutoff =  maxWait;
    batcher -> handle_ = scheduler->schedule(maxWait, [=]() {
      auto maybe_send = batcher->sendBatch();
      if (maybe_send.has_error()) {
        batcher->handle_.reschedule(maxWait);
        return maybe_send.error();
      }
      batcher->handle_.reschedule(maxWait); // TODO: maybe in gsl::finally
    });
    return batcher;
  }

  outcome::result<void> PreCommitBatcherImpl::sendBatch() {
    std::unique_lock<std::mutex> locker(mutex_, std::defer_lock);

    OUTCOME_TRY(head, api_->ChainHead());
    OUTCOME_TRY(minfo, api_->StateMinerInfo(miner_address_, head->key));

    std::vector<SectorPreCommitInfo> params;
    for (auto &data : batchStorage) {
      mutualDeposit += data.second.deposit;
      params.push_back(data.second.pci);
    }
    // TODO: goodFunds = mutualDeposit + MaxFee; -  for checking payable

    OUTCOME_TRY(encodedParams, codec::cbor::encode(params));

    auto maybe_signed = api_->MpoolPushMessage(
        vm::message::UnsignedMessage(
            miner_address_,
            minfo.worker,  // TODO: handle worker
            0,
            mutualDeposit,
            {},
            {},
            vm::actor::builtin::v0::miner::PreCommitSector::Number,
            MethodParams{encodedParams}),
        kPushNoSpec);

    if (maybe_signed.has_error()) {
      return maybe_signed.error();
    }

    mutualDeposit = 0;
    batchStorage.clear();

    cutoffStart = std::chrono::system_clock::now();
    return outcome::success();
  }

  outcome::result<void> PreCommitBatcherImpl::forceSend() {
    handle_.reschedule(0);
    return outcome::success();
  }

  void PreCommitBatcherImpl::getPreCommitCutoff(ChainEpoch curEpoch,
                                                const SectorInfo &si) {
    primitives::ChainEpoch cutoffEpoch =
        si.ticket_epoch + static_cast<int64_t>(fc::kEpochsInDay + 600);
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
      handle_.reschedule(0);
    } else {
      Ticks tempCutoff = toTicks(std::chrono::seconds((cutoffEpoch - curEpoch)
                                                      * kEpochDurationSeconds));
      if ((closestCutoff
               - toTicks(std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now() - cutoffStart))
           > tempCutoff)) {
        cutoffStart = std::chrono::system_clock::now();
        handle_.reschedule(tempCutoff);
        closestCutoff = tempCutoff;
      }
    }
  }

  outcome::result<void> PreCommitBatcherImpl::addPreCommit(
      SectorInfo secInf,
      primitives::TokenAmount deposit,
      api::SectorPreCommitInfo pcInfo) {
    std::unique_lock<std::mutex> locker(mutex_, std::defer_lock);
    OUTCOME_TRY(head, api_->ChainHead());

    getPreCommitCutoff(head->epoch(), secInf);
    auto sn = secInf.sector_number;
    batchStorage[sn] = preCommitEntry(std::move(deposit), std::move(pcInfo));
    return outcome::success();
  }
}  // namespace fc::mining