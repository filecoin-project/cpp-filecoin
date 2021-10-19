/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/mining.hpp"

#include "common/logger.hpp"
#include "common/math/math.hpp"
#include "const.hpp"
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
  using api::DomainSeparationTag;
  using common::math::expneg;
  using common::math::kPrecision256;
  using crypto::randomness::drawRandomness;
  using BlsSignature = crypto::bls::Signature;

  outcome::result<std::shared_ptr<Mining>> Mining::create(
      std::shared_ptr<Scheduler> scheduler,
      std::shared_ptr<UTCClock> clock,
      std::shared_ptr<FullNodeApi> api,
      std::shared_ptr<Prover> prover,
      const Address &miner) {
    OUTCOME_TRY(version, api->Version());
    auto mining{std::make_shared<Mining>()};
    mining->scheduler = scheduler;
    mining->clock = clock;
    mining->api = api;
    mining->prover = prover;
    mining->miner = miner;
    mining->block_delay = version.block_delay;
    mining->propagation =
        std::min<uint64_t>(kPropagationDelaySecs, mining->block_delay * 0.3f);
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
        OUTCOME_LOG("Mining::submit error", submit(std::move(block1)));
      });
    } else {
      ++skip;
      wait(time + propagation, true, [this] { waitParent(); });
    }
    return outcome::success();
  }

  outcome::result<void> Mining::submit(BlockTemplate block1) {
    // TODO: slash filter
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
      ts = std::move(*ts2);
      weight = weight2;
      skip = 0;
    }
    return outcome::success();
  }

  ChainEpoch Mining::height() const {
    return ts->height() + skip + 1;
  }

  void Mining::wait(int64_t sec, bool abs, Scheduler::Callback cb) {
    if (abs) {
      sec -= clock->nowUTC().count();
    }
    cb = [cb{std::move(cb)}, self{shared_from_this()}] {
      // stop condition or weak_ptr
      cb();
    };
    scheduler->schedule(std::move(cb),
                        std::chrono::seconds{std::max<int64_t>(0, sec)});
  }

  constexpr auto kTicketRandomnessLookback{1};
  outcome::result<boost::optional<BlockTemplate>> Mining::prepareBlock() {
    if (info && info->has_min_power) {
      auto vrf{[&](auto tag,
                   auto height,
                   auto &seed) -> outcome::result<BlsSignature> {
        OUTCOME_TRY(
            sig,
            api->WalletSign(
                info->worker,
                copy(drawRandomness(info->beacon().data, tag, height, seed))));
        return boost::get<BlsSignature>(sig);
      }};
      OUTCOME_TRY(miner_seed, codec::cbor::encode(miner));
      OUTCOME_TRY(election_vrf,
                  vrf(DomainSeparationTag::ElectionProofProduction,
                      height(),
                      miner_seed));
      auto win_count{computeWinCount(
          election_vrf, info->miner_power, info->network_power)};
      if (win_count > 0) {
        spdlog::info("height={} win={} power={}% ticket={}",
                     height(),
                     win_count,
                     bigdiv(100 * info->miner_power, info->network_power),
                     common::hex_lower(election_vrf));

        auto ticket_seed{miner_seed};
        if (height() > kUpgradeSmokeHeight) {
          append(ticket_seed, ts->getMinTicketBlock().ticket->bytes);
        }
        OUTCOME_TRY(ticket_vrf,
                    vrf(DomainSeparationTag::TicketProduction,
                        height() - kTicketRandomnessLookback,
                        ticket_seed));
        OUTCOME_TRY(
            post_proof,
            prover->generateWinningPoSt(
                miner.getId(),
                info->sectors,
                drawRandomness(info->beacon().data,
                               DomainSeparationTag::WinningPoStChallengeSeed,
                               height(),
                               miner_seed)));
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

  auto bigblake(BytesIn vrf) {
    auto _hash{crypto::blake2b::blake2b_256(vrf)};
    BigInt hash;
    boost::multiprecision::import_bits(hash, _hash.begin(), _hash.end());
    return hash;
  }

  constexpr auto kMaxWinCount{3 * kBlocksPerEpoch};
  int64_t computeWinCount(BytesIn ticket,
                          const BigInt &power,
                          const BigInt &total_power) {
    auto hash{bigblake(ticket)};
    auto lambda{
        bigdiv((power * kBlocksPerEpoch) << kPrecision256, total_power)};
    auto pmf{expneg(lambda, kPrecision256)};
    BigInt icdf{(BigInt{1} << kPrecision256) - pmf};
    auto k{0};
    while (hash < icdf && k < kMaxWinCount) {
      ++k;
      pmf = (bigdiv(pmf, k) * lambda) >> kPrecision256;
      icdf -= pmf;
    }
    return k;
  }

  double ticketQuality(BytesIn ticket) {
    return 1
           - (double)boost::multiprecision::cpp_rational{bigblake(ticket),
                                                         BigInt{1} << 256};
  }
}  // namespace fc::mining
