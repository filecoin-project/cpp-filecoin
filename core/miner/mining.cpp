/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/mining.hpp"

#include "common/logger.hpp"
#include "primitives/block/rand.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/runtime/pricelist.hpp"

#define OUTCOME_LOG(tag, r)                               \
  {                                                       \
    auto &&_r{r};                                         \
    if (!_r) {                                            \
      spdlog::error("{}: {}", tag, _r.error().message()); \
      return;                                             \
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
    mining->propagation =
        std::min<uint64_t>(kPropagationDelaySecs, mining->block_delay * 3 / 10);
    return mining;
  }

  void Mining::start() {
    waitParent();
  }

  void Mining::waitParent() {
    OUTCOME_LOG("Mining::waitParent error", bestParent());
    wait(ts->getMinTimestamp() + propagation, true, [this] {
      OUTCOME_LOG("Mining::waitBeacon error", waitBeacon());
    });
  }

  outcome::result<void> Mining::waitBeacon() {
    api->BeaconGetEntry(
        [self{shared_from_this()}](auto beacon) {
          OUTCOME_LOG("Mining::waitBeacon error", beacon);
          OUTCOME_LOG("Mining::waitInfo error", self->waitInfo());
        },
        height());
    return outcome::success();
  }

  outcome::result<void> Mining::waitInfo() {
    OUTCOME_TRY(bestParent());
    if (!mined.emplace(ts->key, skip).second) {
      wait(block_delay, false, [this] { waitParent(); });
    } else {
      api->MinerGetBaseInfo(
          [self{shared_from_this()}](auto _info) {
            OUTCOME_LOG("Mining::waitInfo error", _info);
            self->info = std::move(_info.value());
            OUTCOME_LOG("Mining::prepare error", self->prepare());
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
      wait(time, true, [this, block1{std::move(*block1)}]() {
        OUTCOME_LOG("Mining::submit error", submit(block1));
      });
    } else {
      ++skip;
      wait(time + propagation, true, [this] { waitParent(); });
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

  void Mining::wait(uint64_t sec, bool abs, Scheduler::Callback cb) {
    if (abs) {
      sec -= clock->nowUTC().count();
    }
    cb = [cb{std::move(cb)}, self{shared_from_this()}] {
      // stop condition or weak_ptr
      cb();
    };
    scheduler->schedule(std::move(cb),
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
