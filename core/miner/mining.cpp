/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/mining.hpp"

#include "common/logger.hpp"
#include "primitives/block/rand.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/runtime/pricelist.hpp"

#define OUTCOME_TRY_LOG(tag, r)                           \
  {                                                       \
    auto &&_r{r};                                         \
    if (!_r) {                                            \
      spdlog::error("{}: {}", tag, _r.error().message()); \
      return _r.error();                                  \
    }                                                     \
  }

#define OUTCOME_REBOOT(base, tag, r)                      \
  {                                                       \
    auto &&_r{r};                                         \
    if (!_r) {                                            \
      spdlog::error("{}: {}", tag, _r.error().message()); \
      return base->reboot();                              \
    }                                                     \
  }

namespace fc::mining {
  using BlsSignature = crypto::bls::Signature;

  outcome::result<std::shared_ptr<Mining>> Mining::create(
      std::shared_ptr<Scheduler> scheduler,
      std::shared_ptr<UTCClock> clock,
      std::shared_ptr<FullNodeApi> api,
      std::shared_ptr<Prover> prover,
      const Address &miner) {
    OUTCOME_TRY(version, api->Version());
    auto mining{std::make_shared<Mining>()};
    mining->scheduler = std::move(scheduler);
    mining->clock = std::move(clock);
    mining->api = std::move(api);
    mining->prover = std::move(prover);
    mining->miner = miner;
    mining->block_delay = version.block_delay;
    mining->sleep_time = 5;
    mining->propagation =
        std::min<uint64_t>(kPropagationDelaySecs, mining->block_delay * 3 / 10);
    return mining;
  }

  void Mining::start() {
    waitParent();
  }

  void Mining::reboot() {
    wait(sleep_time, false, [weak{weak_from_this()}] {
      if (auto self{weak.lock()}) {
        self->waitParent();
      }
      return outcome::success();
    });
  }

  void Mining::waitParent() {
    OUTCOME_REBOOT(this, "Mining::waitParent error", bestParent());
    wait(ts->getMinTimestamp() + propagation,
         true,
         [weak{weak_from_this()}]() -> outcome::result<void> {
           if (auto self{weak.lock()}) {
             OUTCOME_TRY_LOG("Mining::waitBeacon error", self->waitBeacon());
           }
           return outcome::success();
         });
  }

  outcome::result<void> Mining::waitBeacon() {
    api->BeaconGetEntry(
        [weak{weak_from_this()}](auto beacon) {
          if (auto self{weak.lock()}) {
            OUTCOME_REBOOT(self, "Mining::waitBeacon error", beacon);
            OUTCOME_REBOOT(self, "Mining::waitInfo error", self->waitInfo());
          }
        },
        height());
    return outcome::success();
  }

  outcome::result<void> Mining::waitInfo() {
    OUTCOME_TRY(bestParent());
    auto maybe_mined = std::make_pair(ts->key, skip);
    if (last_mined == maybe_mined) {
      wait(block_delay, false, [weak{weak_from_this()}] {
        if (auto self{weak.lock()}) {
          self->waitParent();
        }
        return outcome::success();
      });
    } else {
      api->MinerGetBaseInfo(
          [weak{weak_from_this()}, mined{std::move(maybe_mined)}](auto _info) {
            if (auto self{weak.lock()}) {
              OUTCOME_REBOOT(self, "Mining::waitInfo error", _info);
              self->info = std::move(_info.value());
              OUTCOME_REBOOT(self, "Mining::prepare error", self->prepare());
              self->last_mined = mined;
            }
          },
          miner,
          height(),
          ts->key);
    }
    return outcome::success();
  }

  outcome::result<void> Mining::prepare() {
    OUTCOME_TRY(block1, prepareBlock());
    auto time{ts->getMinTimestamp() + (skip + 1) * block_delay};
    if (block1) {
      block1->timestamp = time;
      wait(time,
           true,
           [weak{weak_from_this()},
            block1{std::move(*block1)}]() -> outcome::result<void> {
             if (auto self{weak.lock()}) {
               OUTCOME_TRY_LOG("Mining::submit error", self->submit(block1));
             }
             return outcome::success();
           });
    } else {
      ++skip;
      wait(time + propagation, true, [weak{weak_from_this()}] {
        if (auto self{weak.lock()}) {
          self->waitParent();
        }
        return outcome::success();
      });
    }
    return outcome::success();
  }

  outcome::result<void> Mining::submit(const BlockTemplate &block1) {
    // TODO(turuslan): slash filter
    OUTCOME_TRY(block2, api->MinerCreateBlock(block1));
    auto result{api->SyncSubmitBlock(block2)};
    waitParent();
    OUTCOME_TRY(result);
    return outcome::success();
  }

  outcome::result<void> Mining::bestParent() {
    OUTCOME_TRY(ts2, api->ChainHead());
    if (ts) {
      if (ts2->key == ts->key) {
        return outcome::success();
      }
    }
    OUTCOME_TRY(weight2, api->ChainTipSetWeight(ts2->key));
    if (!ts || weight2 > weight) {
      ts = *ts2;
      weight = weight2;
      skip = 0;
    }
    return outcome::success();
  }

  ChainEpoch Mining::height() const {
    return gsl::narrow<ChainEpoch>(ts->height() + skip + 1);
  }

  void Mining::wait(uint64_t sec,
                    bool abs,
                    std::function<outcome::result<void>()> cb) {
    if (abs) {
      sec -= clock->nowUTC().count();
    }
    Scheduler::Callback wrap_cb = [cb{std::move(cb)}, weak{weak_from_this()}] {
      if (cb().has_error()) {
        if (auto self{weak.lock()}) {
          self->reboot();
        }
      }
    };
    scheduler->schedule(std::move(wrap_cb),
                        std::chrono::seconds{std::max<uint64_t>(0, sec)});
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<boost::optional<BlockTemplate>> Mining::prepareBlock() {
    if (info && info->has_min_power) {
      auto vrf{[&](auto &rand) -> outcome::result<BlsSignature> {
        OUTCOME_TRY(sig, api->WalletSign(info->worker, copy(rand)));
        return boost::get<BlsSignature>(sig);
      }};
      const BlockRand rand{
          miner,
          height(),
          info->beacons,
          info->prev_beacon,
          *ts,
      };
      OUTCOME_TRY(election_vrf, vrf(rand.election));
      auto win_count{computeWinCount(
          election_vrf, info->miner_power, info->network_power)};
      if (win_count > 0) {
        spdlog::info("height={} win={} power={}% ticket={}",
                     height(),
                     win_count,
                     bigdiv(100 * info->miner_power, info->network_power),
                     common::hex_lower(election_vrf));

        OUTCOME_TRY(ticket_vrf, vrf(rand.ticket));

        std::vector<primitives::sector::PoStProof> post_proof;
        if (kFakeWinningPost) {
          copy(post_proof.emplace_back().proof,
               common::span::cbytes(kFakeWinningPostStr));
        } else {
          OUTCOME_TRYA(post_proof,
                       prover->generateWinningPoSt(
                           miner.getId(), info->sectors, rand.win));
        }
        OUTCOME_TRY(messages,
                    api->MpoolSelect(ts->key, ticketQuality(ticket_vrf)));
        return BlockTemplate{
            miner,
            ts->key.cids(),
            primitives::block::Ticket{copy(ticket_vrf)},
            primitives::block::ElectionProof{win_count, copy(election_vrf)},
            std::move(info->beacons),
            messages,
            height(),
            {},
            std::move(post_proof),
        };
      }
    }
    return boost::none;
  }

  double ticketQuality(BytesIn ticket) {
    return 1
           - boost::multiprecision::cpp_rational{blakeBigInt(ticket),
                                                 BigInt{1} << 256}
                 .convert_to<double>();
  }
}  // namespace fc::mining
