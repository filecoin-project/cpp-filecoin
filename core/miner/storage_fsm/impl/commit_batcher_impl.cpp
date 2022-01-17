/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "commit_batcher_impl.hpp"
#include <iterator>

namespace fc::mining {
  using vm::actor::builtin::types::miner::kChainFinality;

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
    const SectorNumber &sector_number = sector_info.sector_number;

    /*
     * TODO batch_storage_ and callbacks_ may union DONE
     */
    union_storage_.push(sector_number, aggregate_input, callback);

    // setPreCommitCutoff(head->epoch(), sector_info); TODO same precommit DONE

    if (union_storage_.size() >= max_size_callback_) {
      sendCallbacks();
    }
    return outcome::success();
  }

  void CommitBatcherImpl::forceSend() {}

  void CommitBatcherImpl::sendCallbacks() {
    UnionStorage temp_union_storage(std::move(union_storage_));
    const auto maybe_result = sendBatch();
    for (const auto &[key, pair_storage] : temp_union_storage) {
      pair_storage.commit_callback(maybe_result);
    } // TODO does it work? because I didn`t iterator++

    cutoff_start_ = std::chrono::system_clock::now();
    closest_cutoff_ = max_delay_;

    handle_.reschedule(max_delay_).value();
  }

  outcome::result<CID> CommitBatcherImpl::sendBatch() {}

  void CommitBatcherImpl::setCommitCutoff(const ChainEpoch &current_epoch,
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

  void CommitBatcherImpl::UnionStorage::push(
      const SectorNumber &sector_number,
      const AggregateInput &aggregate_input,
      const CommitCallback &commit_callback) {
    push(sector_number, PairStorage(aggregate_input, commit_callback));
  }

  void CommitBatcherImpl::UnionStorage::push(
      const SectorNumber &sector_number,
      const CommitCallback &commit_callback,
      const AggregateInput &aggregate_input) {
    push(sector_number, PairStorage(aggregate_input, commit_callback));
  }
  void CommitBatcherImpl::UnionStorage::push(const SectorNumber &sector_number,
                                             const PairStorage &pair_storage) {
    std::unique_lock<std::mutex> locker(mutex_);
    storage_[sector_number] = pair_storage;
  }



  CommitBatcherImpl::UnionStorage::UnionStorage(
      CommitBatcherImpl::UnionStorage &&union_storage1) {
    std::unique_lock<std::mutex> locker(mutex_);
    storage_.insert(std::make_move_iterator(union_storage1.storage_.begin()),
                    std::make_move_iterator(union_storage1.storage_.end()));
  }
  size_t CommitBatcherImpl::UnionStorage::size() const {
    return storage_.size();
  }

  CommitBatcherImpl::UnionStorage::PairStorage::PairStorage(
      const AggregateInput &aggregate_input,
      const CommitCallback &commit_callback)
      : aggregate_input(aggregate_input), commit_callback(commit_callback) {}

  CommitBatcherImpl::UnionStorage::PairStorage::PairStorage(
      const CommitCallback &commit_callback,
      const AggregateInput &aggregate_input)
      : aggregate_input(aggregate_input), commit_callback(commit_callback) {}

  std::map<SectorNumber, CommitBatcherImpl::UnionStorage::PairStorage>::iterator
  CommitBatcherImpl::UnionStorage::begin() {
    return storage_.begin();
  }

  std::map<SectorNumber, CommitBatcherImpl::UnionStorage::PairStorage>::iterator
  CommitBatcherImpl::UnionStorage::end() {
    return storage_.end();
  }
}  // namespace fc::mining
