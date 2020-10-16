/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/mining.hpp"

#include "const.hpp"
#include "vm/actor/builtin/market/policy.hpp"
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
  using crypto::randomness::drawRandomness;
  using BlsSignature = crypto::bls::Signature;

  std::shared_ptr<Mining> Mining::create(std::shared_ptr<Scheduler> scheduler,
                                         std::shared_ptr<UTCClock> clock,
                                         std::shared_ptr<Api> api,
                                         std::shared_ptr<Prover> prover,
                                         const Address &miner) {
    auto mining{std::make_shared<Mining>()};
    mining->scheduler = scheduler;
    mining->clock = clock;
    mining->api = api;
    mining->prover = prover;
    mining->miner = miner;
    return mining;
  }

  void Mining::start() {
    waitParent();
  }

  void Mining::waitParent() {
    OUTCOME_LOG("Mining::waitParent error", bestParent());
    wait(ts->getMinTimestamp() + kPropagationDelaySecs, true, [this] {
      OUTCOME_LOG("Mining::waitBeacon error", waitBeacon());
    });
  }

  outcome::result<void> Mining::waitBeacon() {
    OUTCOME_TRY(_wait, api->BeaconGetEntry(height()));
    _wait.wait([self{shared_from_this()}, _wait](auto _beacon) {
      OUTCOME_LOG("Mining::waitBeacon error", _beacon);
      OUTCOME_LOG("Mining::waitInfo error", self->waitInfo());
    });
    return outcome::success();
  }

  outcome::result<void> Mining::waitInfo() {
    OUTCOME_TRY(bestParent());
    OUTCOME_TRY(ts_key, ts->makeKey());
    if (!mined.emplace(ts_key, skip).second) {
      wait(kBlockDelaySecs, false, [this] { waitParent(); });
    } else {
      OUTCOME_TRY(_wait, api->MinerGetBaseInfo(miner, height(), ts_key));
      _wait.wait([self{shared_from_this()}, _wait](auto _info) {
        OUTCOME_LOG("Mining::waitInfo error", _info);
        self->info = std::move(_info.value());
        OUTCOME_LOG("Mining::prepare error", self->prepare());
      });
    }
    return outcome::success();
  }

  outcome::result<void> Mining::prepare() {
    OUTCOME_TRY(block1, prepareBlock());
    auto time{ts->getMinTimestamp() + (skip + 1) * kBlockDelaySecs};
    if (block1) {
      block1->timestamp = time;
      wait(time, true, [this, block1{std::move(*block1)}]() {
        OUTCOME_LOG("Mining::submit error", submit(std::move(block1)));
      });
    } else {
      ++skip;
      wait(time + kPropagationDelaySecs, true, [this] { waitParent(); });
    }
    return outcome::success();
  }

  outcome::result<void> Mining::submit(BlockTemplate block1) {
    // TODO: slash filter
    OUTCOME_TRY(block2, api->MinerCreateBlock(block1));
    OUTCOME_TRY(api->SyncSubmitBlock(block2));
    waitParent();
    return outcome::success();
  }

  outcome::result<void> Mining::bestParent() {
    OUTCOME_TRY(ts2, api->ChainHead());
    OUTCOME_TRY(ts2_key, ts2.makeKey());
    if (ts) {
      OUTCOME_TRY(ts_key, ts->makeKey());
      if (ts2_key == ts_key) {
        return outcome::success();
      }
    }
    OUTCOME_TRY(weight2, api->ChainTipSetWeight(ts2_key));
    if (!ts || weight2 > weight) {
      ts = std::move(ts2);
      weight = weight2;
      skip = 0;
    }
    return outcome::success();
  }

  ChainEpoch Mining::height() const {
    return ts->height + skip + 1;
  }

  void Mining::wait(int64_t sec, bool abs, Scheduler::Callback cb) {
    if (abs) {
      sec -= clock->nowUTC().count();
    }
    cb = [cb{std::move(cb)}, self{shared_from_this()}] {
      // stop condition or weak_ptr
      cb();
    };
    scheduler
        ->schedule(libp2p::protocol::scheduler::toTicks(
                       std::chrono::seconds{std::max<int64_t>(0, sec)}),
                   std::move(cb))
        .detach();
  }

  constexpr auto kTicketRandomnessLookback{1};
  outcome::result<boost::optional<BlockTemplate>> Mining::prepareBlock() {
    if (info && info->has_min_power) {
      auto vrf{[&](auto tag,
                   auto height,
                   auto &seed) -> outcome::result<BlsSignature> {
        OUTCOME_TRY(
            sig,
            api->WalletSign(info->worker,
                            Buffer{drawRandomness(
                                info->beacon().data, tag, height, seed)}));
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
        auto ticket_seed{miner_seed};
        if (height() > kUpgradeSmokeHeight) {
          ticket_seed.put(ts->getMinTicketBlock().ticket->bytes);
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
        OUTCOME_TRY(
            messages,
            api->MpoolSelect(ts->makeKey().value(), ticketQuality(ticket_vrf)));
        return BlockTemplate{
            miner,
            ts->cids,
            primitives::block::Ticket{Buffer{ticket_vrf}},
            primitives::block::ElectionProof{win_count, Buffer{election_vrf}},
            std::move(info->beacons),
            messages,
            (uint64_t)height(),
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
    constexpr auto P{256};
    static auto poly{[P](std::initializer_list<std::string> s) {
      return [P, p{std::vector<BigInt>{s.begin(), s.end()}}](const BigInt &x) {
        BigInt r;
        for (auto &c : p) {
          r = ((r * x) >> P) + c;
        }
        return r;
      };
    }};
    static auto pn{poly({
        "-648770010757830093818553637600",
        "67469480939593786226847644286976",
        "-3197587544499098424029388939001856",
        "89244641121992890118377641805348864",
        "-1579656163641440567800982336819953664",
        "17685496037279256458459817590917169152",
        "-115682590513835356866803355398940131328",
        "340282366920938463463374607431768211456",
    })};
    static auto pd{poly({
        "1225524182432722209606361",
        "114095592300906098243859450",
        "5665570424063336070530214243",
        "194450132448609991765137938448",
        "5068267641632683791026134915072",
        "104716890604972796896895427629056",
        "1748338658439454459487681798864896",
        "23704654329841312470660182937960448",
        "259380097567996910282699886670381056",
        "2250336698853390384720606936038375424",
        "14978272436876548034486263159246028800",
        "72144088983913131323343765784380833792",
        "224599776407103106596571252037123047424",
        "340282366920938463463374607431768211456",
    })};
    auto hash{bigblake(ticket)};
    auto lambda{bigdiv((power * kBlocksPerEpoch) << P, total_power)};
    auto pmf{bigdiv(pn(lambda) << P, pd(lambda))};
    BigInt icdf{(BigInt{1} << P) - pmf};
    auto k{0};
    while (hash < icdf && k < kMaxWinCount) {
      ++k;
      pmf = (bigdiv(pmf, k) * lambda) >> P;
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
