/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/windowpost.hpp"

#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/version/version.hpp"

namespace fc::mining {
  using primitives::sector::RegisteredSealProof;
  using sector_storage::SectorRef;
  using vm::actor::builtin::types::miner::kWPoStPeriodDeadlines;
  using vm::actor::builtin::v0::miner::DeclareFaults;
  using vm::actor::builtin::v0::miner::DeclareFaultsRecovered;

  inline const api::MessageSendSpec kSpec{50 * kFilecoinPrecision};

  outcome::result<std::shared_ptr<WindowPoStScheduler>>
  WindowPoStScheduler::create(std::shared_ptr<FullNodeApi> api,
                              std::shared_ptr<Prover> prover,
                              std::shared_ptr<FaultTracker> fault_tracker,
                              const Address &miner) {
    OUTCOME_TRY(chan, api->ChainNotify());
    auto scheduler{std::make_shared<WindowPoStScheduler>()};
    scheduler->channel = std::move(chan.channel);
    scheduler->api = api;
    scheduler->prover = std::move(prover);
    scheduler->fault_tracker = std::move(fault_tracker);
    scheduler->miner = miner;
    OUTCOME_TRY(info, api->StateMinerInfo(miner, {}));
    OUTCOME_TRYA(scheduler->worker, api->StateAccountKey(info.worker, {}));
    scheduler->part_size = info.window_post_partition_sectors;
    scheduler->proof_type = info.window_post_proof_type;
    scheduler->channel->read([scheduler](auto changes) {
      if (changes) {
        TipsetCPtr revert, apply;
        for (auto &change : *changes) {
          if (change.type == primitives::tipset::HeadChangeType::REVERT) {
            revert = change.value;
          } else {
            apply = change.value;
          }
        }
        scheduler->onChange(revert, apply);
      }
      return true;
    });
    return std::move(scheduler);
  }

  void WindowPoStScheduler::onChange(TipsetCPtr revert, TipsetCPtr apply) {
    DeadlineInfo deadline;
    if (auto _deadline{api->StateMinerProvingDeadline(miner, apply->key)}) {
      deadline = _deadline.value();
    } else {
      return;
    }
    if (!deadline.periodStarted()) {
      return;
    }
    while (cache.count(deadline.open)) {
      deadline = deadline.nextNotElapsed();
    }
    if (apply->epoch() >= deadline.challenge) {
      auto &cached{
          cache.emplace(deadline.open, Cached{deadline, {}, 0}).first->second};

      // TODO: fault cutoff
      auto declare_index{(deadline.index + 2) % kWPoStPeriodDeadlines};
      if (auto _parts{
              api->StateMinerPartitions(miner, declare_index, apply->key)}) {
        auto declare{[&](auto faults) {
          DeclareFaults::Params params;
          uint64_t _part{0};
          for (auto &part : _parts.value()) {
            if (auto _sectors{checkSectors(faults
                                               ? part.live - part.faulty
                                               : part.faulty - part.recovering,
                                           !faults)}) {
              auto &sectors{_sectors.value()};
              if (!sectors.empty()) {
                params.faults.push_back(
                    {declare_index, _part, std::move(sectors)});
              }
            } else {
              return;
            }
            ++_part;
          }
          if (!params.faults.empty()) {
            std::ignore = pushMessage(
                faults ? DeclareFaults::Number : DeclareFaultsRecovered::Number,
                codec::cbor::encode(params).value());
          }
        }};
        declare(false);
        if (apply->epoch() <= kUpgradeIgnitionHeight) {
          declare(true);
        }
      }

      if (auto _parts{
              api->StateMinerPartitions(miner, deadline.index, apply->key)}) {
        auto seed{codec::cbor::encode(miner).value()};
        if (auto _rand{api->ChainGetRandomnessFromBeacon(
                apply->key,
                api::DomainSeparationTag::WindowedPoStChallengeSeed,
                deadline.challenge,
                seed)}) {
          auto prove{[&](auto _part, auto parts) -> outcome::result<void> {
            SubmitWindowedPoSt::Params params;
            params.deadline = deadline.index;
            std::vector<SectorInfo> sectors;
            RleBitset post_skip;
            for (auto &part : parts) {
              auto to_prove{part.live - part.faulty + part.recovering};
              OUTCOME_TRY(good, checkSectors(to_prove, true));
              good = good - post_skip;
              auto skip{to_prove - good};
              OUTCOME_TRY(_sectors,
                          api->StateMinerSectors(miner, good, apply->key));
              if (!_sectors.empty()) {
                std::map<api::SectorNumber, SectorInfo> map;
                for (auto &sector : _sectors) {
                  map.emplace(sector.sector,
                              SectorInfo{
                                  sector.seal_proof,
                                  sector.sector,
                                  sector.sealed_cid,
                              });
                }
                auto sub{map.at(_sectors[0].sector)};
                for (auto id : part.all) {
                  auto it{map.find(id)};
                  if (it == map.end()) {
                    sectors.push_back(sub);
                  } else {
                    sectors.push_back(std::move(it->second));
                  }
                }
                params.partitions.push_back({_part, skip});
              }
              ++_part;
            }
            if (!params.partitions.empty()) {
              // TODO: retry 5 times
              if (auto _proof{prover->generateWindowPoSt(
                      miner.getId(), sectors, _rand.value())}) {
                auto &proof{_proof.value()};
                params.proofs = std::move(proof.proof);
                for (auto &sector : proof.skipped) {
                  post_skip.insert(sector.sector);
                }
                cached.params.push_back(std::move(params));
              }
            }
            return outcome::success();
          }};
          auto parts{gsl::make_span(_parts.value())};
          for (auto _part{0u}; _part < parts.size(); _part += part_size) {
            std::ignore = prove(
                _part,
                parts.subspan(
                    _part, std::min<size_t>(parts.size(), _part + part_size)));
          }
        }
      }
    }

    for (auto &[open, cached] : cache) {
      if (revert && revert->epoch() < open) {
        cached.submitted = 0;
      }
      if (cached.submitted < cached.params.size()
          && apply->epoch() < cached.deadline.close
          && apply->epoch() >= open + kStartConfidence) {
        if (auto _rand{api->ChainGetRandomnessFromTickets(
                apply->key,
                api::DomainSeparationTag::PoStChainCommit,
                open,
                {})}) {
          for (auto &params : cached.params) {
            params.chain_commit_epoch = open;
            params.chain_commit_rand = _rand.value();
            std::ignore = pushMessage(SubmitWindowedPoSt::Number,
                                      codec::cbor::encode(params).value());
          }
          ++cached.submitted;
        }
      }
    }
  }

  outcome::result<RleBitset> WindowPoStScheduler::checkSectors(
      const RleBitset &sectors, bool ok) {
    std::vector<SectorRef> refs;
    for (const auto &id : sectors) {
      refs.push_back({{miner.getId(), id}, RegisteredSealProof::kUndefined});
    }
    OUTCOME_TRY(bad_ids, fault_tracker->checkProvable(proof_type, refs));
    RleBitset bad;
    for (auto &id : bad_ids) {
      bad.insert(id.sector);
    }
    return ok ? sectors - bad : bad;
  }

  outcome::result<void> WindowPoStScheduler::pushMessage(MethodNumber method,
                                                         Buffer params) {
    api::UnsignedMessage msg;
    msg.method = method;
    msg.params = std::move(params);
    msg.to = miner;
    msg.from = worker;
    auto _from{[&]() -> outcome::result<Address> {
      OUTCOME_TRY(info, api->StateMinerInfo(miner, {}));
      OUTCOME_TRYA(msg, api->GasEstimateMessageGas(msg, kSpec, {}));
      auto check{[&](auto &addr) -> outcome::result<void> {
        OUTCOME_TRY(balance, api->WalletBalance(addr));
        if (balance >= msg.value + msg.requiredFunds()) {
          OUTCOME_TRY(key, api->StateAccountKey(addr, {}));
          OUTCOME_TRY(has, api->WalletHas(key));
          if (has) {
            return outcome::success();
          }
        }
        return std::errc::owner_dead;
      }};
      for (auto &addr : info.control) {
        if (check(addr)) {
          return addr;
        }
      }
      if (check(info.owner)) {
        return info.owner;
      }
      return info.worker;
    }()};
    if (_from) {
      msg.from = _from.value();
    }
    OUTCOME_TRY(smsg, api->MpoolPushMessage(msg, kSpec));
    OUTCOME_TRY(api->StateWaitMsg(
        [method](auto _r) {
          auto name{method == DeclareFaultsRecovered::Number
                        ? "DeclareFaultsRecovered"
                        : method == DeclareFaults::Number
                              ? "DeclareFaults"
                              : method == SubmitWindowedPoSt::Number
                                    ? "SubmitWindowedPoSt"
                                    : "(unexpected)"};
          if (!_r) {
            spdlog::error("WindowPoStScheduler {} error {}", name, _r.error());
          } else {
            auto &r{_r.value().receipt};
            if (r.exit_code != vm::VMExitCode::kOk) {
              spdlog::error(
                  "WindowPoStScheduler {} exit {}", name, r.exit_code);
            }
          }
        },
        smsg.getCid(),
        kMessageConfidence,
        api::kLookbackNoLimit,
        true));
    return outcome::success();
  }
}  // namespace fc::mining
