/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/windowpost.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"

namespace fc {
  inline constexpr auto kValue{1000};
  inline constexpr auto kGasPrice{1};
  inline constexpr auto kGasLimit{10000000};

  outcome::result<std::shared_ptr<WindowPoStScheduler>>
  WindowPoStScheduler::create(std::shared_ptr<Api> api,
                              std::shared_ptr<Prover> prover,
                              const Address &miner) {
    OUTCOME_TRY(chan, api->ChainNotify());
    auto scheduler{std::make_shared<WindowPoStScheduler>()};
    scheduler->channel = std::move(chan.channel);
    scheduler->api = api;
    scheduler->prover = std::move(prover);
    scheduler->miner = miner;
    OUTCOME_TRY(info, api->StateMinerInfo(miner, {}));
    OUTCOME_TRYA(scheduler->worker, api->StateAccountKey(info.worker, {}));
    scheduler->part_size = info.window_post_partition_sectors;
    scheduler->channel->read([scheduler](auto changes) {
      if (changes) {
        // single CURRENT or last APPLY
        auto &change{*changes->rbegin()};
        if (change.type != primitives::tipset::HeadChangeType::REVERT) {
          auto result{scheduler->onTipset(*change.value)};
          if (!result) {
            spdlog::warn("WindowPoStScheduler ChainNotify handler: {}",
                         result.error().message());
            // TODO: should stop or continue?
            return false;
          }
        }
      }
      return true;
    });
    return std::move(scheduler);
  }

  outcome::result<void> WindowPoStScheduler::onTipset(const Tipset &ts) {
    OUTCOME_TRY(deadline, api->StateMinerProvingDeadline(miner, ts.key));
    auto same{last_deadline
              && last_deadline->period_start == deadline.period_start
              && last_deadline->index == deadline.index
              && last_deadline->challenge == deadline.challenge};
    if (!same && deadline.periodStarted()
        && deadline.challenge + kStartConfidence
               < static_cast<int64_t>(ts.height())) {
      last_deadline = deadline;

      // TODO: check recoveries and faults

      using vm::actor::builtin::miner::SubmitWindowedPoSt;
      SubmitWindowedPoSt::Params params;
      params.deadline = deadline.index;
      OUTCOME_TRY(parts,
                  api->StateMinerPartitions(miner, deadline.index, ts.key));
      std::map<api::SectorNumber, size_t> part_of;
      std::vector<api::SectorInfo> sectors2;
      for (auto i{0u}; i < parts.size(); ++i) {
        auto &part{parts[i]};
        auto to_prove{part.sectors - part.terminated
                      - (part.faults - part.recoveries)};
        auto good{to_prove};  // TODO: check provable
        OUTCOME_TRY(sectors1,
                    api->StateMinerSectors(miner, good, false, ts.key));
        for (auto &sector : sectors1) {
          part_of.emplace(sector.id, i);
          sectors2.push_back(
              {sector.info.seal_proof, sector.id, sector.info.sealed_cid});
        }
        if (!sectors1.empty()) {
          params.partitions.push_back({i, to_prove - good});
        }
      }
      if (!sectors2.empty()) {
        OUTCOME_TRY(seed, codec::cbor::encode(miner));
        OUTCOME_TRY(rand,
                    api->ChainGetRandomnessFromBeacon(
                        ts.key,
                        api::DomainSeparationTag::WindowedPoStChallengeSeed,
                        deadline.challenge,
                        seed));
        OUTCOME_TRY(proof,
                    prover->generateWindowPoSt(miner.getId(), sectors2, rand));
        params.proofs = std::move(proof.proof);
        for (auto &sector : proof.skipped) {
          params.partitions[part_of.at(sector.sector)].skipped.insert(
              sector.sector);
        }
        params.chain_commit_epoch = deadline.open;
        OUTCOME_TRYA(params.chain_commit_rand,
                     api->ChainGetRandomnessFromTickets(
                         ts.key,
                         api::DomainSeparationTag::PoStChainCommit,
                         params.chain_commit_epoch,
                         {}));
        OUTCOME_TRY(api->MpoolPushMessage(
            {
                miner,
                worker,
                0,
                kValue,
                kGasPrice,
                kGasLimit,
                SubmitWindowedPoSt::Number,
                codec::cbor::encode(params).value(),
            },
            api::kPushNoSpec));
        // TODO: should wait result and retry?
      }
    }
    return outcome::success();
  }
}  // namespace fc
