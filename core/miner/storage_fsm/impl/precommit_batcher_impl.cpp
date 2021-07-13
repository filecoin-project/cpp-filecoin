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

  PreCommitBatcherImpl::PreCommitEntry::PreCommitEntry(
      TokenAmount number, api::SectorPreCommitInfo info)
      : deposit(std::move(number)), precommit_info(std::move(info)){};

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

    batcher->cutoffStart = std::chrono::system_clock::now();
    batcher->closestCutoff = maxWait;
    batcher->handle_ = scheduler->schedule(maxWait, [=]() {
      auto maybe_send = batcher->sendBatch();
      batcher->handle_.reschedule(maxWait);  // TODO: maybe in gsl::finally
    });
    return batcher;
  }

  outcome::result<void> PreCommitBatcherImpl::sendBatch() {
    std::unique_lock<std::mutex> locker(mutex_, std::defer_lock);
    // TODO(Elestrias): [FIL-398] goodFunds = mutualDeposit + MaxFee; -  for checking payable
    if (batchStorage.size() != 0) {
      auto head = api_->ChainHead().value();
      auto minfo = api_->StateMinerInfo(miner_address_, head->key).value();

      PreCommitBatch::Params params = {};
      for (const auto &data : batchStorage) {
        mutualDeposit += data.second.deposit;
        params.sectors.push_back(data.second.precommit_info);
      }
      OUTCOME_TRY(encodedParams, codec::cbor::encode(params));

      auto maybe_signed = api_->MpoolPushMessage(
          vm::message::UnsignedMessage(
              miner_address_,
              minfo.worker,  // TODO: handle worker
              0,
              mutualDeposit,
              {},
              {},
              25,  // TODO (m.tagirov) Miner actor v5 PreCommitBatch
              MethodParams{encodedParams}),
          kPushNoSpec);

      if (maybe_signed.has_error()) {
        return maybe_signed.error(); //TODO: maybe logs
      }

      mutualDeposit = 0;
      batchStorage.clear();
    }
    cutoffStart = std::chrono::system_clock::now();
    return outcome::success();
  }

  outcome::result<void> PreCommitBatcherImpl::forceSend() {
    OUTCOME_TRY(sendBatch());
    handle_.reschedule(maxDelay);
    return outcome::success();
  }

  outcome::result<void> PreCommitBatcherImpl::setPreCommitCutoff(ChainEpoch curEpoch,
                                                const SectorInfo &si) {
    ChainEpoch cutoffEpoch =
        si.ticket_epoch + static_cast<int64_t>(fc::kEpochsInDay + kChainFinality);
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
      if ((closestCutoff
               - toTicks(std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now() - cutoffStart))
           > tempCutoff)) {
        cutoffStart = std::chrono::system_clock::now();
        handle_.reschedule(tempCutoff);
        closestCutoff = tempCutoff;
      }
    }
    return outcome::success();
  }

  outcome::result<void> PreCommitBatcherImpl::addPreCommit(
      SectorInfo secInf,
      TokenAmount deposit,
      api::SectorPreCommitInfo pcInfo) {
    std::unique_lock<std::mutex> locker(mutex_, std::defer_lock);
    OUTCOME_TRY(head, api_->ChainHead());

    auto sn = secInf.sector_number;
    batchStorage[sn] = PreCommitEntry(std::move(deposit), std::move(pcInfo));

    setPreCommitCutoff(head->epoch(), secInf);
    return outcome::success();
  }
}  // namespace fc::mining
