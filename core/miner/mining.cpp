/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/mining.hpp"
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

  std::shared_ptr<Mining> Mining::create(std::shared_ptr<io_context> io,
                                         std::shared_ptr<Api> api,
                                         std::shared_ptr<Prover> prover,
                                         const Address &miner) {
    auto mining{std::make_shared<Mining>()};
    mining->io = io;
    mining->timer = std::make_unique<system_timer>(*io);
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
    timer->expires_at(system_timer::time_point{
        std::chrono::seconds{ts->getMinTimestamp()} + kPropagationDelay});
    asyncWait([self{shared_from_this()}]() {
      OUTCOME_LOG("Mining::prepare error", self->prepare());
    });
  }

  outcome::result<void> Mining::prepare() {
    OUTCOME_TRY(bestParent());
    OUTCOME_TRY(ts_key, ts->makeKey());
    if (mined.emplace(ts_key, skip).second) {
      OUTCOME_TRY(block1,
                  prepareBlock(miner, *ts, ts->height + skip + 1, api, prover));
      timer->expires_at(
          system_timer::time_point{std::chrono::seconds{ts->getMinTimestamp()}
                                   + (skip + 1) * kBlockDelay});
      if (block1) {
        OUTCOME_TRYA(block1->messages, selectMessages(api, *ts));
        asyncWait([self{shared_from_this()}, block1{std::move(*block1)}]() {
          OUTCOME_LOG("Mining::submit error", self->submit(std::move(block1)));
        });
        return outcome::success();
      } else {
        ++skip;
      }
    } else {
      timer->expires_after(kBlockDelay);
    }
    asyncWait([self{shared_from_this()}]() { self->waitParent(); });
    return outcome::success();
  }

  outcome::result<void> Mining::submit(BlockTemplate block1) {
    block1.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
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

  constexpr auto kBlocksPerEpoch{5};
  bool isTicketWinner(BytesIn ticket,
                      const BigInt &power,
                      const BigInt &total_power) {
    auto _hash{crypto::blake2b::blake2b_256(ticket)};
    BigInt hash;
    boost::multiprecision::import_bits(hash, _hash.begin(), _hash.end());
    return hash * total_power < (power << 256) * kBlocksPerEpoch;
  }

  constexpr auto kTicketRandomnessLookback{1};
  outcome::result<boost::optional<BlockTemplate>> prepareBlock(
      const Address &miner,
      const Tipset &ts,
      uint64_t height,
      std::shared_ptr<Api> api,
      std::shared_ptr<Prover> prover) {
    assert(height > ts.height);
    OUTCOME_TRY(ts_key, ts.makeKey());
    OUTCOME_TRY(info, api->MinerGetBaseInfo(miner, height, ts_key));
    if (info && info->miner_power > 0) {
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
                      height,
                      miner_seed));
      if (isTicketWinner(
              election_vrf, info->miner_power, info->network_power)) {
        auto ticket_seed{miner_seed};
        if (info->beacons.empty()) {
          ticket_seed.put(ts.getMinTicketBlock().ticket->bytes);
        }
        OUTCOME_TRY(ticket_vrf,
                    vrf(DomainSeparationTag::TicketProduction,
                        height - kTicketRandomnessLookback,
                        ticket_seed));
        OUTCOME_TRY(
            post_proof,
            prover->generateWinningPoSt(
                miner.getId(),
                info->sectors,
                drawRandomness(info->beacon().data,
                               DomainSeparationTag::WinningPoStChallengeSeed,
                               height,
                               miner_seed)));
        return BlockTemplate{
            miner,
            ts.cids,
            primitives::block::Ticket{ticket_vrf},
            primitives::block::ElectionProof{Buffer{election_vrf}},
            std::move(info->beacons),
            {},
            height,
            {},
            std::move(post_proof),
        };
      }
    }
    return boost::none;
  }

  constexpr auto kBlockMessageLimit{512};
  constexpr auto kBlockGasLimit{100000000};
  outcome::result<std::vector<SignedMessage>> selectMessages(
      std::shared_ptr<Api> api, const Tipset &ts) {
    OUTCOME_TRY(ts_key, ts.makeKey());
    OUTCOME_TRY(messages1, api->MpoolPending(ts_key));
    std::vector<SignedMessage> messages2;
    vm::runtime::Pricelist pricelist;
    std::map<Address, vm::actor::Actor> actors;
    for (auto &_msg : messages1) {
      auto &msg{_msg.message};
      if (msg.version != vm::message::kMessageVersion
          || msg.gasLimit > kBlockGasLimit || msg.value < 0
          || msg.value > vm::actor::builtin::market::kTotalFilecoin
          || msg.gasPrice < 0) {
        continue;
      }
      OUTCOME_TRY(message_bytes,
                  _msg.signature.isBls() ? codec::cbor::encode(msg)
                                         : codec::cbor::encode(_msg));
      if (msg.gasLimit < pricelist.onChainMessage(message_bytes.size())) {
        continue;
      }
      auto _actor{actors.find(msg.from)};
      if (_actor == actors.end()) {
        OUTCOME_TRY(actor, api->StateGetActor(msg.from, ts_key));
        _actor = actors.emplace(msg.from, std::move(actor)).first;
      }
      auto &actor{_actor->second};
      if (msg.nonce != actor.nonce || actor.balance < msg.requiredFunds()) {
        continue;
      }
      actor.balance -= msg.requiredFunds();
      ++actor.nonce;
      messages2.push_back(std::move(_msg));
      if (messages2.size() > kBlockMessageLimit) {
        break;
      }
    }
    return messages2;
  }
}  // namespace fc::mining
