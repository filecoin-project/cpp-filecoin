/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/windowpost.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"

namespace fc {
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
          auto result{scheduler->onTipset(change.value)};
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
    OUTCOME_TRY(ts_key, ts.makeKey());
    OUTCOME_TRY(deadline, api->StateMinerProvingDeadline(miner, ts_key));
    auto same{last_deadline
              && last_deadline->period_start == deadline.period_start
              && last_deadline->index == deadline.index
              && last_deadline->challenge == deadline.challenge};
    if (!same && deadline.periodStarted()
        && deadline.challenge + kStartConfidence
               < static_cast<int64_t>(ts.height)) {
      last_deadline = deadline;
      OUTCOME_TRY(deadlines, api->StateMinerDeadlines(miner, ts_key));
      auto parts{deadlines.count(part_size, deadline.index).first};
      if (parts != 0) {
        using vm::actor::builtin::miner::SubmitWindowedPoSt;
        SubmitWindowedPoSt::Params params;
        auto first_part{deadlines.partitions(part_size, deadline.index).first};
        for (auto i{0u}; i < parts; ++i) {
          params.partitions.push_back(first_part + i);
        }
        OUTCOME_TRY(sectors1,
                    api->StateMinerSectors(
                        miner, deadlines.due[deadline.index], false, ts_key));
        std::vector<api::SectorInfo> sectors2;
        for (auto &sector : sectors1) {
          sectors2.push_back({sector.info.info.registered_proof,
                              sector.id,
                              sector.info.info.sealed_cid});
        }
        OUTCOME_TRY(seed, codec::cbor::encode(miner));
        OUTCOME_TRY(rand,
                    api->ChainGetRandomness(
                        ts_key,
                        api::DomainSeparationTag::WindowedPoStChallengeSeed,
                        deadline.challenge,
                        seed));
        OUTCOME_TRY(proof,
                    prover->generateWindowPoSt(miner.getId(), sectors2, rand));
        params.proofs = std::move(proof.proof);
        OUTCOME_TRY(api->MpoolPushMessage({
            miner,
            worker,
            0,
            1000,
            1,
            10000000,
            SubmitWindowedPoSt::Number,
            codec::cbor::encode(params).value(),
        }));
        // TODO: should wait result and retry?
      }
    }
    return outcome::success();
  }
}  // namespace fc
