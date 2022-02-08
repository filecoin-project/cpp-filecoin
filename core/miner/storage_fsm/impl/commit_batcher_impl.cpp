/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "commit_batcher_impl.hpp"
#include <iostream>
#include <utility>
#include "vm/actor/builtin/v5/miner/miner_actor.hpp"
#include "vm/actor/builtin/v6/monies.hpp"

#include <string>

namespace fc::mining {
  using api::kPushNoSpec;
  using fc::BytesIn;
  using fc::proofs::ProofEngine;
  using primitives::ActorId;
  using primitives::go::bigdiv;
  using primitives::sector::AggregateSealVerifyInfo;
  using primitives::sector::AggregateSealVerifyProofAndInfos;
  using primitives::tipset::TipsetCPtr;
  using vm::actor::MethodParams;
  using vm::actor::builtin::types::miner::kChainFinality;
  using vm::actor::builtin::types::miner::SectorPreCommitOnChainInfo;
  using vm::actor::builtin::v5::miner::ProveCommitAggregate;
  using vm::actor::builtin::v6::miner::AggregateProveCommitNetworkFee;

  CommitBatcherImpl::CommitBatcherImpl(const std::chrono::seconds &max_time,
                                       std::shared_ptr<FullNodeApi> api,
                                       Address miner_address,
                                       std::shared_ptr<Scheduler> scheduler,
                                       AddressSelector address_selector,
                                       std::shared_ptr<FeeConfig> fee_config,
                                       const size_t &max_size_callback,
                                       std::shared_ptr<ProofEngine> proof)
      : scheduler_(std::move(scheduler)),
        max_delay_(max_time),
        closest_cutoff_(max_time),
        max_size_callback_(max_size_callback),
        api_(std::move(api)),
        miner_address_(std::move(miner_address)),
        fee_config_(std::move(fee_config)),
        proof_(std::move(proof)),
        address_selector_(std::move(address_selector)) {
    cutoff_start_ = std::chrono::system_clock::now();
    sendCallbacks(max_delay_);
  }

  outcome::result<void> CommitBatcherImpl::addCommit(
      const SectorInfo &sector_info,
      const AggregateInput &aggregate_input,
      const CommitCallback &callback) {
    std::unique_lock<std::mutex> locker(mutex_storage_);

    const SectorNumber &sector_number = sector_info.sector_number;
    OUTCOME_TRY(head, api_->ChainHead());

    pair_storage_[sector_number] = PairStorage{aggregate_input, callback};

    if (pair_storage_.size() >= max_size_callback_) {
      locker.unlock();
      forceSend();
      return outcome::success();
    }

    setCommitCutoff(head->epoch(), sector_info);
    return outcome::success();
  }

  void CommitBatcherImpl::forceSend() {
    MapPairStorage pair_storage_for_send_;

    std::unique_lock<std::mutex> locker(mutex_storage_);
    pair_storage_for_send_ = std::move(pair_storage_);
    locker.unlock();

    const auto maybe_result = sendBatch(pair_storage_for_send_);

    for (auto &[key, pair_storage] : pair_storage_for_send_) {
      pair_storage.commit_callback(maybe_result);
    }

    cutoff_start_ = std::chrono::system_clock::now();
    closest_cutoff_ = max_delay_;
    sendCallbacks(max_delay_);
  }

  void CommitBatcherImpl::sendCallbacks(std::chrono::milliseconds time) {
    handle_ = scheduler_->scheduleWithHandle(
        [&]() {
          MapPairStorage pair_storage_for_send_;

          std::unique_lock<std::mutex> locker(mutex_storage_);
          pair_storage_for_send_ = std::move(pair_storage_);
          locker.unlock();

          const auto maybe_result = sendBatch(pair_storage_for_send_);

          for (auto &[key, pair_storage] : pair_storage_for_send_) {
            pair_storage.commit_callback(maybe_result);
          }

          cutoff_start_ = std::chrono::system_clock::now();
          closest_cutoff_ = max_delay_;

          handle_.reschedule(max_delay_).value();
        },
        time);
  }

  outcome::result<CID> CommitBatcherImpl::sendBatch(
      const MapPairStorage &pair_storage_for_send) {
    if (pair_storage_for_send.empty()) {
      cutoff_start_ = std::chrono::system_clock::now();
      return ERROR_TEXT("Empty Batcher");
    }
    OUTCOME_TRY(head, api_->ChainHead());

    const size_t total = pair_storage_for_send.size();

    ProveCommitAggregate::Params params;

    std::vector<Proof> proofs;
    proofs.reserve(total);

    std::vector<AggregateSealVerifyInfo> infos;
    infos.reserve(total);

    BigInt collateral = 0;

    for (const auto &[sector_number, pair_storage] : pair_storage_for_send) {
      OUTCOME_TRY(sector_collateral,
                  getSectorCollateral(sector_number, head->key));
      collateral = collateral + sector_collateral;

      params.sectors.insert(sector_number);
      infos.push_back(pair_storage.aggregate_input.info);
    }

    for (const auto &[sector_number, pair_storage] : pair_storage_for_send) {
      proofs.push_back(pair_storage.aggregate_input.proof);
    }

    const ActorId mid = miner_address_.getId();
    // TODO maybe long (AggregateSealProofs)
    AggregateSealVerifyProofAndInfos aggregate_seal{
        .miner = mid,
        .seal_proof =
            pair_storage_for_send.at(infos[0].number).aggregate_input.spt,
        .aggregate_proof = arp_,
        .infos = infos};

    std::vector<BytesIn> proofsSpan;
    proofsSpan.reserve(proofs.size());

    for (const Proof &proof : proofs) {
      proofsSpan.push_back(gsl::make_span(proof));
    }

    OUTCOME_TRY(proof_->aggregateSealProofs(aggregate_seal, proofsSpan));

    params.proof = aggregate_seal.proof;
    OUTCOME_TRY(encode, codec::cbor::encode(params));
    OUTCOME_TRY(miner_info, api_->StateMinerInfo(miner_address_, head->key));

    const TokenAmount max_fee =
        fee_config_->max_commit_batch_gas_fee.FeeForSector(proofs.size());

    OUTCOME_TRY(tipset, api_->ChainGetTipSet(head->key));
    const BigInt base_fee = tipset->blks[0].parent_base_fee;

    TokenAmount agg_fee_raw =
        AggregateProveCommitNetworkFee(infos.size(), base_fee);

    TokenAmount agg_fee = bigdiv(agg_fee_raw * agg_fee_num_, agg_fee_den_);
    TokenAmount need_funds = collateral + agg_fee;
    TokenAmount good_funds = max_fee + need_funds;

    OUTCOME_TRY(address,
                address_selector_(miner_info, good_funds, need_funds, api_));
    OUTCOME_TRY(signed_messege,
                api_->MpoolPushMessage(
                    vm::message::UnsignedMessage(miner_address_,
                                                 address,
                                                 0,
                                                 need_funds,
                                                 max_fee,
                                                 {},
                                                 ProveCommitAggregate::Number,
                                                 MethodParams{encode}),
                    kPushNoSpec));

    cutoff_start_ = std::chrono::system_clock::now();
    return signed_messege.getCid();
  }

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
        sendCallbacks(temp_cutoff);
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
