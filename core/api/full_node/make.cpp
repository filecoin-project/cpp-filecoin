/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/full_node/make.hpp"

#include <boost/algorithm/string.hpp>
#include <condition_variable>
#include <libp2p/peer/peer_id.hpp>

#include "api/version.hpp"
#include "blockchain/production/block_producer.hpp"
#include "cbor_blake/ipld_version.hpp"
#include "common/logger.hpp"
#include "const.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"
#include "drand/beaconizer.hpp"
#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "node/pubsub_gate.hpp"
#include "primitives/tipset/chain.hpp"
#include "proofs/impl/proof_engine_impl.hpp"
#include "storage/car/car.hpp"
#include "vm/actor/builtin/states/account/account_actor_state.hpp"
#include "vm/actor/builtin/states/init/init_actor_state.hpp"
#include "vm/actor/builtin/states/market/market_actor_state.hpp"
#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/states/reward/reward_actor_state.hpp"
#include "vm/actor/builtin/states/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/states/verified_registry/verified_registry_actor_state.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/actor/builtin/types/storage_power/policy.hpp"
#include "vm/actor/builtin/v0/market/market_actor.hpp"
#include "vm/actor/builtin/v5/market/validate.hpp"
#include "vm/actor/builtin/v5/miner/monies.hpp"
#include "vm/interpreter/interpreter.hpp"
#include "vm/message/impl/message_signer_impl.hpp"
#include "vm/message/message.hpp"
#include "vm/runtime/env.hpp"
#include "vm/runtime/impl/tipset_randomness.hpp"
#include "vm/state/impl/state_tree_impl.hpp"
#include "vm/toolchain/toolchain.hpp"

#define MOVE(x)  \
  x {            \
    std::move(x) \
  }

#define FWD(x)                   \
  x {                            \
    std::forward<decltype(x)>(x) \
  }

namespace fc::api {
  using connection_t = boost::signals2::connection;
  using InterpreterResult = vm::interpreter::Result;
  using common::Logger;
  using crypto::randomness::DomainSeparationTag;
  using crypto::signature::BlsSignature;
  using libp2p::peer::PeerId;
  using markets::retrieval::DealProposalParams;
  using markets::retrieval::QueryResponse;
  using primitives::kChainEpochUndefined;
  using primitives::block::MsgMeta;
  using primitives::sector::getPreferredSealProofTypeFromWindowPoStType;
  using primitives::tipset::Tipset;
  using storage::ipld::kAllSelector;
  using vm::isVMExitCode;
  using vm::VMExitCode;
  using vm::actor::kInitAddress;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::kStoragePowerAddress;
  using vm::actor::kVerifiedRegistryAddress;
  using vm::actor::builtin::states::AccountActorStatePtr;
  using vm::actor::builtin::states::InitActorStatePtr;
  using vm::actor::builtin::states::MarketActorStatePtr;
  using vm::actor::builtin::states::MinerActorStatePtr;
  using vm::actor::builtin::states::PowerActorStatePtr;
  using vm::actor::builtin::states::RewardActorStatePtr;
  using vm::actor::builtin::states::VerifiedRegistryActorStatePtr;
  using vm::actor::builtin::types::market::DealState;
  using vm::actor::builtin::types::storage_power::kConsensusMinerMinPower;
  using vm::interpreter::InterpreterCache;
  using vm::message::kDefaultGasLimit;
  using vm::message::kDefaultGasPrice;
  using vm::runtime::Env;
  using vm::state::StateTreeImpl;
  using vm::toolchain::Toolchain;
  using vm::version::getNetworkVersion;

  const static Logger logger = common::createLogger("Full node API");

  // TODO (turuslan): reuse for block validation
  inline bool minerHasMinPower(const StoragePower &claim_qa,
                               size_t min_power_miners) {
    return min_power_miners < kConsensusMinerMinMiners
               ? !claim_qa.is_zero()
               : claim_qa > kConsensusMinerMinPower;
  }

  void beaconEntriesForBlock(const DrandSchedule &schedule,
                             Beaconizer &beaconizer,
                             ChainEpoch epoch,
                             drand::Round prev,
                             const CbT<std::vector<BeaconEntry>> &cb) {
    auto max{schedule.maxRound(epoch)};
    if (max == prev) {
      return cb(outcome::success());
    }
    auto round{prev == 0 ? max : prev + 1};
    auto async{std::make_shared<AsyncAll<BeaconEntry>>(max - round + 1, cb)};
    for (auto i{0u}; round <= max; ++round, ++i) {
      beaconizer.entry(round, async->on(i));
    }
  }

  struct TipsetContext {
    TipsetCPtr tipset;
    StateTreeImpl state_tree;
    boost::optional<InterpreterResult> interpreted;

    auto marketState() -> outcome::result<MarketActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(kStorageMarketAddress));
      return getCbor<MarketActorStatePtr>(state_tree.getStore(), actor.head);
    }

    auto minerState(const Address &address)
        -> outcome::result<MinerActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(address));
      return getCbor<MinerActorStatePtr>(state_tree.getStore(), actor.head);
    }

    auto powerState() -> outcome::result<PowerActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(kStoragePowerAddress));
      return getCbor<PowerActorStatePtr>(state_tree.getStore(), actor.head);
    }

    auto rewardState() -> outcome::result<RewardActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(vm::actor::kRewardAddress));
      return getCbor<RewardActorStatePtr>(state_tree.getStore(), actor.head);
    }

    auto initState() -> outcome::result<InitActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(kInitAddress));
      return getCbor<InitActorStatePtr>(state_tree.getStore(), actor.head);
    }

    auto verifiedRegistryState()
        -> outcome::result<VerifiedRegistryActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(kVerifiedRegistryAddress));
      return getCbor<VerifiedRegistryActorStatePtr>(state_tree.getStore(),
                                                    actor.head);
    }

    outcome::result<Address> accountKey(const Address &id) {
      OUTCOME_TRY(actor, state_tree.get(id));
      OUTCOME_TRY(
          state,
          getCbor<AccountActorStatePtr>(state_tree.getStore(), actor.head));
      return state->address;
    }

    // TODO (a.chernyshov) make explicit
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator IpldPtr() const {
      return state_tree.getStore();
    }
  };

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<std::vector<SectorInfo>> getSectorsForWinningPoSt(
      NetworkVersion network,
      const Address &miner,
      const MinerActorStatePtr &state,
      const Randomness &post_rand,
      const IpldPtr &ipld) {
    std::vector<SectorInfo> sectors;
    RleBitset sectors_bitset;
    OUTCOME_TRY(deadlines, state->deadlines.get());
    for (const auto &deadline_cid : deadlines.due) {
      OUTCOME_TRY(deadline, deadline_cid.get());
      OUTCOME_TRY(deadline->partitions.visit([&](auto, auto &part) {
        for (auto sector : part->sectors) {
          if (network >= NetworkVersion::kVersion7) {
            if (part->terminated.has(sector) || part->unproven.has(sector)) {
              continue;
            }
          }
          if (!part->faults.has(sector)) {
            sectors_bitset.insert(sector);
          }
        }
        return outcome::success();
      }));
    }
    if (!sectors_bitset.empty()) {
      OUTCOME_TRY(miner_info, state->getInfo());
      OUTCOME_TRY(
          win_type,
          getRegisteredWinningPoStProof(miner_info->window_post_proof_type));
      static auto proofs{std::make_shared<proofs::ProofEngineImpl>()};
      OUTCOME_TRY(
          indices,
          proofs->generateWinningPoStSectorChallenge(
              win_type, miner.getId(), post_rand, sectors_bitset.size()));
      std::vector<uint64_t> sector_ids{sectors_bitset.begin(),
                                       sectors_bitset.end()};
      for (auto &i : indices) {
        OUTCOME_TRY(sector, state->sectors.sectors.get(sector_ids[i]));
        sectors.push_back(
            {sector.seal_proof, sector.sector, sector.sealed_cid});
      }
    }
    return sectors;
  }

  // NOLINTNEXTLINE(hicpp-function-size,readability-function-cognitive-complexity,readability-function-size,google-readability-function-size)
  std::shared_ptr<FullNodeApi> makeImpl(
      std::shared_ptr<FullNodeApi> api,
      const std::shared_ptr<ChainStore> &chain_store,
      const IpldPtr &markets_ipld,
      const std::string &network_name,
      const std::shared_ptr<WeightCalculator> &weight_calculator,
      const EnvironmentContext &env_context,
      TsBranchPtr ts_main,
      const std::shared_ptr<MessagePool> &mpool,
      const std::shared_ptr<MsgWaiter> &msg_waiter,
      const std::shared_ptr<Beaconizer> &beaconizer,
      const std::shared_ptr<DrandSchedule> &drand_schedule,
      const std::shared_ptr<PubSubGate> &pubsub,
      const std::shared_ptr<KeyStore> &key_store,
      const std::shared_ptr<Discovery> &market_discovery,
      const std::shared_ptr<RetrievalClient> &retrieval_market_client,
      const std::shared_ptr<OneKey> &wallet_default_address) {
    auto ts_load{env_context.ts_load};
    auto ipld{env_context.ipld};
    auto interpreter_cache{env_context.interpreter_cache};
    auto tipsetContext = [=](const TipsetKey &tipset_key,
                             bool interpret =
                                 false) -> outcome::result<TipsetContext> {
      TipsetCPtr tipset;
      if (tipset_key.cids().empty()) {
        tipset = chain_store->heaviestTipset();
      } else {
        OUTCOME_TRYA(tipset, ts_load->load(tipset_key));
      }
      auto ipld{withVersion(env_context.ipld, tipset->height())};
      TipsetContext context{tipset, {ipld, tipset->getParentStateRoot()}, {}};
      if (interpret) {
        OUTCOME_TRY(result, interpreter_cache->get(tipset->key));
        context.state_tree = {ipld, result.state_root};
        context.interpreted = result;
      }
      return context;
    };

    api->AuthNew = [](auto) { return Buffer{1, 2, 3}; };
    api->BeaconGetEntry = [=](auto &&cb, auto epoch) {
      return beaconizer->entry(drand_schedule->maxRound(epoch), cb);
    };
    api->ChainGetBlock = [=](auto &block_cid) {
      return getCbor<BlockHeader>(ipld, block_cid);
    };
    api->ChainGetBlockMessages =
        [=](auto &block_cid) -> outcome::result<BlockMessages> {
      BlockMessages messages;
      OUTCOME_TRY(block, getCbor<BlockHeader>(ipld, block_cid));
      OUTCOME_TRY(meta, getCbor<MsgMeta>(ipld, block.messages));
      OUTCOME_TRY(meta.bls_messages.visit(
          [&](auto, auto &cid) -> outcome::result<void> {
            OUTCOME_TRY(message, getCbor<UnsignedMessage>(ipld, cid));
            messages.bls.push_back(std::move(message));
            messages.cids.push_back(cid);
            return outcome::success();
          }));
      OUTCOME_TRY(meta.secp_messages.visit(
          [&](auto, auto &cid) -> outcome::result<void> {
            OUTCOME_TRY(message, getCbor<SignedMessage>(ipld, cid));
            messages.secp.push_back(std::move(message));
            messages.cids.push_back(cid);
            return outcome::success();
          }));
      return messages;
    };
    api->ChainGetGenesis = [=]() -> outcome::result<TipsetCPtr> {
      return ts_load->lazyLoad(ts_main->bottom().second);
    };
    api->ChainGetNode = [=](auto &path) -> outcome::result<IpldObject> {
      std::vector<std::string> parts;
      boost::split(parts, path, [](auto c) { return c == '/'; });
      if (parts.size() < 3 || !parts[0].empty() || parts[1] != "ipfs") {
        return ERROR_TEXT("ChainGetNode: invalid path");
      }
      OUTCOME_TRY(root, CID::fromString(parts[2]));
      return getNode(ipld, root, gsl::make_span(parts).subspan(3));
    };
    api->ChainGetMessage = [=](auto &cid) -> outcome::result<UnsignedMessage> {
      OUTCOME_TRY(cbor, ipld->get(cid));
      return UnsignedMessage::decode(cbor);
    };
    api->ChainGetParentMessages =
        [=](auto &block_cid) -> outcome::result<std::vector<CidMessage>> {
      std::vector<CidMessage> messages;
      OUTCOME_TRY(block, getCbor<BlockHeader>(ipld, block_cid));
      for (auto &parent_cid : block.parents) {
        OUTCOME_TRY(parent, getCbor<BlockHeader>(ipld, CID{parent_cid}));
        OUTCOME_TRY(meta, getCbor<MsgMeta>(ipld, parent.messages));
        OUTCOME_TRY(meta.bls_messages.visit(
            [&](auto, auto &cid) -> outcome::result<void> {
              OUTCOME_TRY(message, getCbor<UnsignedMessage>(ipld, cid));
              messages.push_back({cid, std::move(message)});
              return outcome::success();
            }));
        OUTCOME_TRY(meta.secp_messages.visit(
            [&](auto, auto &cid) -> outcome::result<void> {
              OUTCOME_TRY(message, getCbor<SignedMessage>(ipld, cid));
              messages.push_back({cid, std::move(message.message)});
              return outcome::success();
            }));
      }
      return messages;
    };
    api->ChainGetParentReceipts =
        [=](auto &block_cid) -> outcome::result<std::vector<MessageReceipt>> {
      OUTCOME_TRY(block, getCbor<BlockHeader>(ipld, block_cid));
      return adt::Array<MessageReceipt>{block.parent_message_receipts, ipld}
          .values();
    };
    api->ChainGetRandomnessFromBeacon =
        [=](auto &tipset_key,
            auto tag,
            auto epoch,
            auto &entropy) -> outcome::result<Randomness> {
      std::shared_lock ts_lock{*env_context.ts_branches_mutex};
      OUTCOME_TRY(ts_branch, TsBranch::make(ts_load, tipset_key, ts_main));
      return env_context.randomness->getRandomnessFromBeacon(
          ts_branch, tag, epoch, entropy);
    };
    api->ChainGetRandomnessFromTickets =
        [=](auto &tipset_key,
            auto tag,
            auto epoch,
            auto &entropy) -> outcome::result<Randomness> {
      std::shared_lock ts_lock{*env_context.ts_branches_mutex};
      OUTCOME_TRY(ts_branch, TsBranch::make(ts_load, tipset_key, ts_main));
      return env_context.randomness->getRandomnessFromTickets(
          ts_branch, tag, epoch, entropy);
    };
    api->ChainGetTipSet = [=](auto &tipset_key) {
      return ts_load->load(tipset_key);
    };
    api->ChainGetTipSetByHeight =
        [=](auto height, auto &tipset_key) -> outcome::result<TipsetCPtr> {
      std::shared_lock ts_lock{*env_context.ts_branches_mutex};
      OUTCOME_TRY(ts_branch, TsBranch::make(ts_load, tipset_key, ts_main));
      OUTCOME_TRY(it, find(ts_branch, height));
      return ts_load->lazyLoad(it.second->second);
    };
    api->ChainHead = [=]() { return chain_store->heaviestTipset(); };
    api->ChainNotify = [=]() {
      auto channel = std::make_shared<Channel<std::vector<HeadChange>>>();
      auto cnn = std::make_shared<connection_t>();
      *cnn = chain_store->subscribeHeadChanges([=](auto &changes) {
        if (!channel->write({changes})) {
          assert(cnn->connected());
          cnn->disconnect();
        }
      });
      return Chan{std::move(channel)};
    };
    api->ChainReadObj = [=](const auto &cid) { return ipld->get(cid); };
    // TODO(turuslan): FIL-165 implement method
    api->ChainSetHead =
        std::function<decltype(api->ChainSetHead)::FunctionSignature>{};
    api->ChainTipSetWeight =
        [=](auto &tipset_key) -> outcome::result<TipsetWeight> {
      OUTCOME_TRY(tipset, ts_load->load(tipset_key));
      return weight_calculator->calculateWeight(*tipset);
    };

    auto retrievalQuery{[=](auto &&cb,
                            const Address &miner,
                            const CID &root,
                            const boost::optional<CID> &piece) {
      OUTCOME_CB(auto minfo, api->StateMinerInfo(miner, {}));
      RetrievalPeer peer;
      peer.address = miner;
      OUTCOME_CB(peer.peer_id, PeerId::fromBytes(minfo.peer_id));
      retrieval_market_client->query(peer, {root, {piece}}, [=](auto _res) {
        OUTCOME_CB(auto res, _res);
        std::string error;
        switch (res.response_status) {
          case markets::retrieval::QueryResponseStatus::kQueryResponseAvailable:
            break;
          case markets::retrieval::QueryResponseStatus::
              kQueryResponseUnavailable:
            error = "retrieval query offer was unavailable: " + res.message;
            break;
          case markets::retrieval::QueryResponseStatus::kQueryResponseError:
            error = "retrieval query offer errored: " + res.message;
            break;
        }
        cb(QueryOffer{
            error,
            root,
            piece,
            res.item_size,
            res.unseal_price + res.min_price_per_byte * res.item_size,
            res.unseal_price,
            res.payment_interval,
            res.interval_increase,
            res.payment_address,
            peer,
        });
      });
    }};

    api->ClientFindData = [=](auto &&cb, auto &&root_cid, auto &&piece_cid) {
      OUTCOME_CB(auto peers, market_discovery->getPeers(root_cid));
      // if piece_cid is specified, remove peers without the piece_cid
      if (piece_cid.has_value()) {
        peers.erase(std::remove_if(peers.begin(),
                                   peers.end(),
                                   [&piece_cid](auto &peer) {
                                     return peer.piece != *piece_cid;
                                   }),
                    peers.end());
      }

      auto waiter = std::make_shared<
          AsyncWaiter<RetrievalPeer, outcome::result<QueryOffer>>>(
          peers.size(), [=](auto all_calls) {
            std::vector<QueryOffer> result;
            for (const auto &[peer, maybe_response] : all_calls) {
              if (maybe_response.has_error()) {
                logger->error("Error when query peer {}",
                              maybe_response.error().message());
              } else {
                result.emplace_back(maybe_response.value());
              }
            }
            cb(result);
          });
      for (const auto &peer : peers) {
        retrievalQuery(waiter->on(peer), peer.address, root_cid, piece_cid);
      }
    };

    // TODO(turuslan): FIL-165 implement method
    api->ClientHasLocal =
        std::function<decltype(api->ClientHasLocal)::FunctionSignature>{};
    api->ClientMinerQueryOffer = retrievalQuery;
    // TODO(turuslan): FIL-165 implement method
    api->ClientQueryAsk =
        std::function<decltype(api->ClientQueryAsk)::FunctionSignature>{};

    /**
     * Initiates the retrieval deal of a file in retrieval market.
     */
    api->ClientRetrieve = [=](auto &&cb, auto order, auto &&file_ref) {
      if (!file_ref.is_car) {
        return cb(ERROR_TEXT("ClientRetrieve unixfs not implemented"));
      }
      if (order.size == 0) {
        return cb(ERROR_TEXT("Cannot make retrieval deal for zero bytes"));
      }
      auto price_per_byte = bigdiv(order.total, order.size);
      DealProposalParams params{
          .selector = kAllSelector,
          .piece = order.piece,
          .price_per_byte = price_per_byte,
          .payment_interval = order.payment_interval,
          .payment_interval_increase = order.payment_interval_increase,
          .unseal_price = order.unseal_price};
      if (!order.peer) {
        OUTCOME_CB(auto info, api->StateMinerInfo(order.miner, {}));
        OUTCOME_CB(auto id, PeerId::fromBytes(info.peer_id));
        order.peer = RetrievalPeer{order.miner, std::move(id), {}};
      }
      OUTCOME_CB1(retrieval_market_client->retrieve(
          order.root,
          params,
          order.total,
          *order.peer,
          order.client,
          order.miner,
          [=](outcome::result<void> res) {
            if (res.has_error()) {
              logger->error("Error in ClientRetrieve {}",
                            res.error().message());
              return cb(res.error());
            }
            logger->info("retrieval deal done");
            OUTCOME_CB1(storage::car::makeSelectiveCar(
                *markets_ipld, {{order.root, {}}}, file_ref.path));
            cb(outcome::success());
          }));
    };

    api->ClientStartDeal =
        std::function<decltype(api->ClientStartDeal)::FunctionSignature>{
            /* impemented in node/main.cpp */};

    api->GasEstimateFeeCap = [=](auto &msg, auto max_blocks, auto &) {
      return mpool->estimateFeeCap(msg.gas_premium, max_blocks);
    };
    api->GasEstimateGasPremium = [=](auto max_blocks, auto &, auto, auto &) {
      return mpool->estimateGasPremium(max_blocks);
    };
    api->GasEstimateMessageGas = [=](auto msg, auto &spec, auto &tsk)
        -> outcome::result<UnsignedMessage> {
      if (msg.from.isId()) {
        OUTCOME_TRY(context, tipsetContext(tsk));
        OUTCOME_TRYA(msg.from,
                     vm::runtime::resolveKey(
                         context.state_tree, context, msg.from, false));
      }
      OUTCOME_TRY(mpool->estimate(
          msg, spec ? spec->max_fee : storage::mpool::kDefaultMaxFee));
      return msg;
    };

    api->MarketReserveFunds = [=](const Address &wallet,
                                  const Address &address,
                                  const TokenAmount &amount)
        -> outcome::result<boost::optional<CID>> {
      if (!amount) {
        return boost::none;
      }
      // TODO(a.chernyshov): method should use fund manager batch reserve
      // and release funds requests for market actor.
      OUTCOME_TRY(
          encoded_params,
          codec::cbor::encode(
              vm::actor::builtin::v0::market::AddBalance::Params{address}));
      UnsignedMessage unsigned_message{
          kStorageMarketAddress,
          wallet,
          {},
          amount,
          0,
          0,
          // TODO (a.chernyshov) there is v0 actor method number, but the
          // actor methods do not depend on version. Should be changed to
          // general method number when methods number are made general
          vm::actor::builtin::v0::market::AddBalance::Number,
          encoded_params};
      OUTCOME_TRY(signed_message,
                  api->MpoolPushMessage(unsigned_message, api::kPushNoSpec));
      return signed_message.getCid();
    };

    api->MinerCreateBlock = [=](auto &t) -> outcome::result<BlockWithCids> {
      OUTCOME_TRY(context, tipsetContext(t.parents, true));
      OUTCOME_TRY(miner_state, context.minerState(t.miner));
      OUTCOME_TRY(block,
                  blockchain::production::generate(
                      *interpreter_cache, ts_load, ipld, std::move(t)));

      OUTCOME_TRY(block_signable, codec::cbor::encode(block.header));
      OUTCOME_TRY(miner_info, miner_state->getInfo());
      OUTCOME_TRY(worker_key, context.accountKey(miner_info->worker));
      OUTCOME_TRY(block_sig, key_store->sign(worker_key, block_signable));
      block.header.block_sig = block_sig;

      BlockWithCids block2;
      block2.header = block.header;
      for (auto &msg : block.bls_messages) {
        OUTCOME_TRY(cid, setCbor(ipld, msg));
        block2.bls_messages.emplace_back(std::move(cid));
      }
      for (auto &msg : block.secp_messages) {
        OUTCOME_TRY(cid, setCbor(ipld, msg));
        block2.secp_messages.emplace_back(std::move(cid));
      }
      return block2;
    };
    api->MinerGetBaseInfo =
        // NOLINTNEXTLINE(readability-function-cognitive-complexity)
        [=](auto &&cb, auto &&miner, auto epoch, auto &&tipset_key) {
          OUTCOME_CB(auto context, tipsetContext(tipset_key, true));
          MiningBaseInfo info;

          std::shared_lock ts_lock{*env_context.ts_branches_mutex};
          OUTCOME_CB(auto ts_branch,
                     TsBranch::make(ts_load, tipset_key, ts_main));
          OUTCOME_CB(auto it, find(ts_branch, context.tipset->height()));
          OUTCOME_CB(info.prev_beacon, latestBeacon(ts_load, it));
          OUTCOME_CB(auto it2, getLookbackTipSetForRound(it, epoch));
          OUTCOME_CB(auto cached,
                     interpreter_cache->get(it2.second->second.key));
          ts_lock.unlock();

          auto prev{info.prev_beacon.round};
          beaconEntriesForBlock(
              *drand_schedule,
              *beaconizer,
              epoch,
              prev,
              [=, FWD(cb), MOVE(context), MOVE(info)](auto _beacons) mutable {
                OUTCOME_CB(info.beacons, _beacons);
                TipsetContext lookback{
                    nullptr,
                    {withVersion(ipld, epoch), std::move(cached.state_root)},
                    {}};
                OUTCOME_CB(auto actor, lookback.state_tree.tryGet(miner));
                if (!actor) {
                  return cb(boost::none);
                }
                OUTCOME_CB(auto miner_state,
                           getCbor<MinerActorStatePtr>(lookback, actor->head));
                OUTCOME_CB(auto seed, codec::cbor::encode(miner));
                auto post_rand{crypto::randomness::drawRandomness(
                    info.beacon().data,
                    DomainSeparationTag::WinningPoStChallengeSeed,
                    epoch,
                    seed)};
                OUTCOME_CB(info.sectors,
                           getSectorsForWinningPoSt(
                               getNetworkVersion(context.tipset->epoch()),
                               miner,
                               miner_state,
                               post_rand,
                               lookback));
                if (info.sectors.empty()) {
                  return cb(boost::none);
                }
                OUTCOME_CB(auto power_state, lookback.powerState());
                OUTCOME_CB(auto claim, power_state->getClaim(miner));
                info.miner_power = claim->qa_power;
                info.network_power = power_state->total_qa_power;
                OUTCOME_CB(auto miner_info, miner_state->getInfo());
                OUTCOME_CB(info.worker, context.accountKey(miner_info->worker));
                info.sector_size = miner_info->sector_size;
                info.has_min_power = minerHasMinPower(
                    claim->qa_power, power_state->num_miners_meeting_min_power);
                cb(std::move(info));
              });
        };
    api->MpoolPending =
        [=](auto &tipset_key) -> outcome::result<std::vector<SignedMessage>> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      if (context.tipset->height() > chain_store->heaviestTipset()->height()) {
        return ERROR_TEXT("MpoolPending: tipset from future requested");
      }
      return mpool->pending();
    };
    api->MpoolPushMessage = [=](auto message,
                                auto &spec) -> outcome::result<SignedMessage> {
      OUTCOME_TRY(context, tipsetContext({}));
      if (message.from.isId()) {
        OUTCOME_TRYA(message.from,
                     vm::runtime::resolveKey(
                         context.state_tree, ipld, message.from, false));
      }
      OUTCOME_TRY(mpool->estimate(
          message, spec ? spec->max_fee : storage::mpool::kDefaultMaxFee));
      OUTCOME_TRYA(message.nonce, mpool->nonce(message.from));
      OUTCOME_TRY(signed_message,
                  vm::message::MessageSignerImpl{key_store}.sign(message.from,
                                                                 message));
      OUTCOME_TRY(mpool->add(signed_message));
      return std::move(signed_message);
    };
    api->MpoolSelect = [=](auto &tsk, auto ticket_quality)
        -> outcome::result<std::vector<SignedMessage>> {
      OUTCOME_TRY(ts, ts_load->load(tsk));
      return mpool->select(ts, ticket_quality);
    };
    api->MpoolSub = [=]() {
      auto channel{std::make_shared<Channel<MpoolUpdate>>()};
      auto cnn{std::make_shared<connection_t>()};
      *cnn = mpool->subscribe([=](auto &change) {
        if (!channel->write(change)) {
          assert(cnn->connected());
          cnn->disconnect();
        }
      });
      return Chan{std::move(channel)};
    };
    api->StateAccountKey = [=](auto &address,
                               auto &tipset_key) -> outcome::result<Address> {
      if (address.isKeyType()) {
        return address;
      }
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      return context.accountKey(address);
    };
    api->StateCall = [=](auto message,
                         auto &tipset_key) -> outcome::result<InvocResult> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));

      std::shared_lock ts_lock{*env_context.ts_branches_mutex};
      OUTCOME_TRY(ts_branch, TsBranch::make(ts_load, tipset_key, ts_main));
      ts_lock.unlock();

      if (!message.gas_limit) {
        message.gas_limit = kBlockGasLimit;
      }
      auto env = std::make_shared<Env>(env_context, ts_branch, context.tipset);
      InvocResult result;
      result.message = message;
      OUTCOME_TRYA(result.receipt, env->applyImplicitMessage(message));
      return result;
    };
    api->StateDealProviderCollateralBounds =
        [=](auto size,
            auto verified,
            auto &tsk) -> outcome::result<DealCollateralBounds> {
      OUTCOME_TRY(context, tipsetContext(tsk));
      OUTCOME_TRY(power, context.powerState());
      OUTCOME_TRY(reward, context.rewardState());
      OUTCOME_TRY(
          circ,
          env_context.circulating->circulating(
              std::make_shared<StateTreeImpl>(std::move(context.state_tree)),
              context.tipset->epoch()));
      auto bounds{
          vm::actor::builtin::types::market::dealProviderCollateralBounds(
              size,
              verified,
              power->total_raw_power,
              power->total_qa_power,
              reward->this_epoch_baseline_power,
              circ,
              getNetworkVersion(context.tipset->epoch()))};
      return DealCollateralBounds{bigdiv(bounds.min * 110, 100), bounds.max};
    };
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    api->StateListMessages = [=](auto &match, auto &tipset_key, auto to_height)
        -> outcome::result<std::vector<CID>> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));

      // TODO(artyom-yurin): Make sure at least one of 'to' or 'from' is
      // defined

      auto matchFunc = [&](const UnsignedMessage &message) -> bool {
        if (match.to != message.to) {
          return false;
        }

        if (match.from != message.from) {
          return false;
        }

        return true;
      };

      std::vector<CID> result;

      while (static_cast<int64_t>(context.tipset->height()) >= to_height) {
        std::set<CID> visited_cid;

        auto isDuplicateMessage = [&](const CID &cid) -> bool {
          return !visited_cid.insert(cid).second;
        };

        for (const BlockHeader &block : context.tipset->blks) {
          OUTCOME_TRY(meta, getCbor<MsgMeta>(ipld, block.messages));
          OUTCOME_TRY(meta.bls_messages.visit(
              [&](auto, auto &cid) -> outcome::result<void> {
                OUTCOME_TRY(message, getCbor<UnsignedMessage>(ipld, cid));

                if (!isDuplicateMessage(cid) && matchFunc(message)) {
                  result.push_back(cid);
                }

                return outcome::success();
              }));
          OUTCOME_TRY(meta.secp_messages.visit(
              [&](auto, auto &cid) -> outcome::result<void> {
                OUTCOME_TRY(message, getCbor<SignedMessage>(ipld, cid));

                if (!isDuplicateMessage(cid) && matchFunc(message.message)) {
                  result.push_back(cid);
                }
                return outcome::success();
              }));
        }

        if (context.tipset->height() == 0) break;

        OUTCOME_TRY(parent_context,
                    tipsetContext(context.tipset->getParents()));

        context = std::move(parent_context);
      }

      return result;
    };
    api->StateGetActor = [=](auto &address,
                             auto &tipset_key) -> outcome::result<Actor> {
      OUTCOME_TRY(context, tipsetContext(tipset_key, true));
      return context.state_tree.get(address);
    };
    api->StateReadState = [=](auto &actor,
                              auto &tipset_key) -> outcome::result<ActorState> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      auto cid = actor.head;
      OUTCOME_TRY(raw, context.state_tree.getStore()->get(cid));
      return ActorState{
          .balance = actor.balance,
          .state = IpldObject{std::move(cid), std::move(raw)},
      };
    };
    api->StateListMiners =
        [=](auto &tipset_key) -> outcome::result<std::vector<Address>> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(power_state, context.powerState());
      return power_state->claims.keys();
    };
    api->StateListActors =
        [=](auto &tsk) -> outcome::result<std::vector<Address>> {
      OUTCOME_TRY(context, tipsetContext(tsk));
      OUTCOME_TRY(root, context.state_tree.flush());
      OUTCOME_TRY(info, getCbor<StateTreeImpl::StateRoot>(context, root));
      adt::Map<Actor, adt::AddressKeyer> actors{info.actor_tree_root, context};

      return actors.keys();
    };
    api->StateMarketBalance =
        [=](auto &address, auto &tipset_key) -> outcome::result<MarketBalance> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(state, context.marketState());
      OUTCOME_TRY(id_address, context.state_tree.lookupId(address));
      OUTCOME_TRY(escrow, state->escrow_table.tryGet(id_address));
      OUTCOME_TRY(locked, state->locked_table.tryGet(id_address));
      if (!escrow) {
        escrow = 0;
      }
      if (!locked) {
        locked = 0;
      }
      return MarketBalance{*escrow, *locked};
    };
    api->StateMarketDeals =
        [=](auto &tipset_key) -> outcome::result<MarketDealMap> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(state, context.marketState());
      MarketDealMap map;
      OUTCOME_TRY(state->proposals.visit(
          [&](auto deal_id, auto &deal) -> outcome::result<void> {
            OUTCOME_TRY(deal_state, state->states.get(deal_id));
            map.emplace(std::to_string(deal_id), StorageDeal{deal, deal_state});
            return outcome::success();
          }));
      return map;
    };
    api->StateLookupID = [=](auto &address,
                             auto &tipset_key) -> outcome::result<Address> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      return context.state_tree.lookupId(address);
    };
    api->StateMarketStorageDeal =
        [=](auto deal_id, auto &tipset_key) -> outcome::result<StorageDeal> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(state, context.marketState());
      OUTCOME_TRY(deal, state->proposals.get(deal_id));
      OUTCOME_TRY(deal_state, state->states.tryGet(deal_id));
      if (!deal_state) {
        deal_state = DealState{
            kChainEpochUndefined, kChainEpochUndefined, kChainEpochUndefined};
      }
      return StorageDeal{deal, *deal_state};
    };
    api->StateMinerActiveSectors =
        [=](auto &miner,
            auto &tsk) -> outcome::result<std::vector<SectorOnChainInfo>> {
      OUTCOME_TRY(context, tipsetContext(tsk));
      OUTCOME_TRY(state, context.minerState(miner));
      std::vector<SectorOnChainInfo> sectors;
      OUTCOME_TRY(deadlines, state->deadlines.get());
      for (const auto &deadline_cid : deadlines.due) {
        OUTCOME_TRY(deadline, deadline_cid.get());
        OUTCOME_TRY(deadline->partitions.visit(
            [&](auto, const auto &part) -> outcome::result<void> {
              for (const auto &id : part->activeSectors()) {
                OUTCOME_TRYA(sectors.emplace_back(),
                             state->sectors.sectors.get(id));
              }
              return outcome::success();
            }));
      }
      return sectors;
    };
    api->StateMinerAvailableBalance =
        [=](auto &miner, auto &tsk) -> outcome::result<TokenAmount> {
      OUTCOME_TRY(context, tipsetContext(tsk));
      OUTCOME_TRY(actor, context.state_tree.get(miner));
      OUTCOME_TRY(miner_state,
                  getCbor<MinerActorStatePtr>(context, actor.head));
      OUTCOME_TRY(vested,
                  miner_state->checkVestedFunds(context.tipset->height()));
      OUTCOME_TRY(available, miner_state->getAvailableBalance(actor.balance));
      return vested + available;
    };
    api->StateMinerDeadlines =
        [=](auto &address,
            auto &tipset_key) -> outcome::result<std::vector<Deadline>> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(state, context.minerState(address));
      OUTCOME_TRY(deadlines, state->deadlines.get());
      std::vector<Deadline> result;
      for (const auto &deadline_cid : deadlines.due) {
        OUTCOME_TRY(deadline, deadline_cid.get());
        result.push_back(Deadline{deadline->partitions_posted});
      }
      return result;
    };
    api->StateMinerFaults = [=](auto address,
                                auto tipset_key) -> outcome::result<RleBitset> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(state, context.minerState(address));
      OUTCOME_TRY(deadlines, state->deadlines.get());
      RleBitset faults;
      for (const auto &deadline_cid : deadlines.due) {
        OUTCOME_TRY(deadline, deadline_cid.get());
        OUTCOME_TRY(deadline->partitions.visit([&](auto, auto &part) {
          faults += part->faults;
          return outcome::success();
        }));
      }
      return faults;
    };
    api->StateMinerInfo = [=](auto &address,
                              auto &tipset_key) -> outcome::result<MinerInfo> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(miner_state, context.minerState(address));
      OUTCOME_TRY(miner_info, miner_state->getInfo());
      return MinerInfo{
          .owner = miner_info->owner,
          .worker = miner_info->worker,
          .control = miner_info->control,
          .peer_id = miner_info->peer_id,
          .multiaddrs = miner_info->multiaddrs,
          .window_post_proof_type = miner_info->window_post_proof_type,
          .sector_size = miner_info->sector_size,
          .window_post_partition_sectors =
              miner_info->window_post_partition_sectors,
      };
    };
    api->StateMinerPartitions = [=](auto &miner, auto _deadline, auto &tsk)
        -> outcome::result<std::vector<Partition>> {
      OUTCOME_TRY(context, tipsetContext(tsk));
      OUTCOME_TRY(state, context.minerState(miner));
      OUTCOME_TRY(deadlines, state->deadlines.get());
      OUTCOME_TRY(deadline, deadlines.due[_deadline].get());
      std::vector<Partition> parts;
      OUTCOME_TRY(deadline->partitions.visit([&](auto, auto &v) {
        parts.push_back({
            v->sectors,
            v->faults,
            v->recoveries,
            v->sectors - v->terminated,
            v->sectors - v->terminated - v->faults,
        });
        return outcome::success();
      }));
      return parts;
    };
    api->StateMinerPower =
        [=](auto &address, auto &tipset_key) -> outcome::result<MinerPower> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(power_state, context.powerState());
      OUTCOME_TRY(miner_power, power_state->getClaim(address));
      Claim total(power_state->total_raw_power, power_state->total_qa_power);

      return MinerPower{*miner_power, total};
    };
    api->StateMinerProvingDeadline =
        [=](auto &address, auto &tipset_key) -> outcome::result<DeadlineInfo> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(state, context.minerState(address));
      const auto deadline_info = state->deadlineInfo(context.tipset->height());
      return deadline_info.nextNotElapsed();
    };
    api->StateMinerSectorAllocated =
        [=](auto &miner, auto sector, auto &tsk) -> outcome::result<bool> {
      OUTCOME_TRY(context, tipsetContext(tsk));
      OUTCOME_TRY(state, context.minerState(miner));
      OUTCOME_TRY(sectors, state->allocated_sectors.get());
      return sectors.has(sector);
    };
    api->StateMinerSectors = [=](auto &address, auto &filter, auto &tipset_key)
        -> outcome::result<std::vector<SectorOnChainInfo>> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(state, context.minerState(address));
      std::vector<SectorOnChainInfo> sectors;
      OUTCOME_TRY(state->sectors.sectors.visit([&](auto id, auto &info) {
        if (!filter || filter->count(id)) {
          sectors.push_back(info);
        }
        return outcome::success();
      }));
      return sectors;
    };
    api->StateNetworkName = [=]() -> outcome::result<std::string> {
      return network_name;
    };
    api->StateNetworkVersion =
        [=](auto &tipset_key) -> outcome::result<NetworkVersion> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      return getNetworkVersion(context.tipset->height());
    };
    constexpr auto kInitialPledgeNum{110};
    constexpr auto kInitialPledgeDen{100};
    api->StateMinerPreCommitDepositForPower =
        [=](auto &miner,
            const SectorPreCommitInfo &precommit,
            auto &tsk) -> outcome::result<TokenAmount> {
      OUTCOME_TRY(context, tipsetContext(tsk));
      OUTCOME_TRY(sector_size, getSectorSize(precommit.registered_proof));
      OUTCOME_TRY(market, context.marketState());
      // TODO(m.tagirov): older market actor versions
      OUTCOME_TRY(
          weights,
          vm::actor::builtin::v5::market::validate(market,
                                                   miner,
                                                   precommit.deal_ids,
                                                   context.tipset->epoch(),
                                                   precommit.expiration));
      const auto weight{vm::actor::builtin::types::miner::qaPowerForWeight(
          sector_size,
          precommit.expiration - context.tipset->epoch(),
          weights.space_time,
          weights.space_time_verified)};
      OUTCOME_TRY(power, context.powerState());
      OUTCOME_TRY(reward, context.rewardState());
      // TODO(m.tagirov): older miner actor versions
      return kInitialPledgeNum
             * vm::actor::builtin::v5::miner::preCommitDepositForPower(
                 reward->this_epoch_reward_smoothed,
                 power->this_epoch_qa_power_smoothed,
                 weight)
             / kInitialPledgeDen;
    };
    api->StateMinerInitialPledgeCollateral =
        [=](auto &miner,
            const SectorPreCommitInfo &precommit,
            auto &tsk) -> outcome::result<TokenAmount> {
      OUTCOME_TRY(context, tipsetContext(tsk));
      OUTCOME_TRY(sector_size, getSectorSize(precommit.registered_proof));
      OUTCOME_TRY(market, context.marketState());
      // TODO(m.tagirov): older market actor versions
      OUTCOME_TRY(
          weights,
          vm::actor::builtin::v5::market::validate(market,
                                                   miner,
                                                   precommit.deal_ids,
                                                   context.tipset->epoch(),
                                                   precommit.expiration));
      const auto weight{vm::actor::builtin::types::miner::qaPowerForWeight(
          sector_size,
          precommit.expiration - context.tipset->epoch(),
          weights.space_time,
          weights.space_time_verified)};
      OUTCOME_TRY(power, context.powerState());
      OUTCOME_TRY(reward, context.rewardState());
      OUTCOME_TRY(
          circ,
          env_context.circulating->circulating(
              std::make_shared<StateTreeImpl>(std::move(context.state_tree)),
              context.tipset->epoch()));
      // TODO(m.tagirov): older miner actor versions
      return kInitialPledgeNum
             * vm::actor::builtin::v5::miner::initialPledgeForPower(
                 circ,
                 reward->this_epoch_reward_smoothed,
                 power->this_epoch_qa_power_smoothed,
                 weight,
                 reward->this_epoch_baseline_power)
             / kInitialPledgeDen;
    };

    api->GetProofType = [=](const Address &miner_address,
                            const TipsetKey &tipset_key)
        -> outcome::result<RegisteredSealProof> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(miner_state, context.minerState(miner_address));
      OUTCOME_TRY(miner_info, miner_state->getInfo());
      const auto network_version = getNetworkVersion(context.tipset->height());
      return getPreferredSealProofTypeFromWindowPoStType(
          network_version, miner_info->window_post_proof_type);
    };

    // TODO(artyom-yurin): FIL-165 implement method
    api->StateSectorGetInfo =
        [=](auto address, auto sector_number, auto tipset_key)
        -> outcome::result<boost::optional<SectorOnChainInfo>> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(state, context.minerState(address));
      return state->sectors.sectors.tryGet(sector_number);
    };
    // TODO(artyom-yurin): FIL-165 implement method
    api->StateSectorPartition =
        std::function<decltype(api->StateSectorPartition)::FunctionSignature>{};

    api->StateVerifiedClientStatus = [=](const Address &address,
                                         const TipsetKey &tipset_key)
        -> outcome::result<boost::optional<StoragePower>> {
      OUTCOME_TRY(context, tipsetContext(tipset_key, true));
      OUTCOME_TRY(id, context.state_tree.lookupId(address));
      OUTCOME_TRY(state, context.verifiedRegistryState());
      return state->getVerifiedClientDataCap(id);
    };

    api->StateSearchMsg =
        [=](auto &&cb, auto &&tsk, auto &&cid, auto &&lookback_limit, bool) {
          OUTCOME_CB(auto context, tipsetContext(tsk));
          msg_waiter->search(
              context.tipset,
              cid,
              lookback_limit,
              [=, FWD(cb)](auto ts, auto receipt) {
                if (!ts) {
                  return cb(boost::none);
                }
                cb(MsgWait{cid, std::move(receipt), ts->key, ts->epoch()});
              });
        };
    api->StateWaitMsg = [=](auto &&cb,
                            auto &&cid,
                            auto &&confidence,
                            auto &&lookback_limit,
                            bool) {
      msg_waiter->wait(
          cid, lookback_limit, confidence, [=, FWD(cb)](auto ts, auto receipt) {
            if (!ts) {
              return cb(ERROR_TEXT("StateWaitMsg not found"));
            }
            cb(MsgWait{cid, std::move(receipt), ts->key, ts->epoch()});
          });
    };
    api->SyncSubmitBlock = [=](auto block) -> outcome::result<void> {
      // TODO(turuslan): chain store must validate blocks before adding
      MsgMeta meta;
      cbor_blake::cbLoadT(ipld, meta);
      for (auto &cid : block.bls_messages) {
        OUTCOME_TRY(meta.bls_messages.append(cid));
      }
      for (auto &cid : block.secp_messages) {
        OUTCOME_TRY(meta.secp_messages.append(cid));
      }
      OUTCOME_TRY(messages, setCbor(ipld, meta));
      if (block.header.messages != messages) {
        return ERROR_TEXT("SyncSubmitBlock: messages cid doesn't match");
      }
      OUTCOME_TRY(chain_store->addBlock(block.header));
      OUTCOME_TRY(pubsub->publish(block));
      return outcome::success();
    };
    api->Version = []() {
      return VersionResult{
          "fuhon", makeApiVersion(2, 1, 0), kEpochDurationSeconds};
    };
    api->WalletBalance = [=](auto &address) -> outcome::result<TokenAmount> {
      OUTCOME_TRY(context, tipsetContext({}));
      OUTCOME_TRY(actor, context.state_tree.tryGet(address));
      if (actor) {
        return actor->balance;
      }
      return 0;
    };
    api->WalletDefaultAddress = [=]() -> outcome::result<Address> {
      if (!wallet_default_address->has())
        return ERROR_TEXT("WalletDefaultAddress: default wallet is not set");
      return wallet_default_address->getCbor<Address>();
    };
    api->WalletHas = [=](auto address) -> outcome::result<bool> {
      if (!address.isKeyType()) {
        OUTCOME_TRY(context, tipsetContext({}));
        OUTCOME_TRYA(address, context.accountKey(address));
      }
      return key_store->has(address);
    };
    api->WalletImport = {[=](auto &info) {
      return key_store->put(info.type, info.private_key);
    }};
    api->WalletNew = {[=](auto &type) -> outcome::result<Address> {
      Address address;
      if (type == "bls") {
        OUTCOME_TRYA(address,
                     key_store->put(crypto::signature::Type::kBls,
                                    crypto::bls::BlsProviderImpl{}
                                        .generateKeyPair()
                                        .value()
                                        .private_key));
      } else if (type == "secp256k1") {
        OUTCOME_TRYA(address,
                     key_store->put(crypto::signature::Type::kSecp256k1,
                                    crypto::secp256k1::Secp256k1ProviderImpl{}
                                        .generate()
                                        .value()
                                        .private_key));
      } else {
        return ERROR_TEXT("WalletNew: unknown type");
      }
      if (!wallet_default_address->has()) {
        wallet_default_address->setCbor(address);
      }
      return std::move(address);
    }};
    api->WalletSetDefault = [=](auto &address) -> outcome::result<void> {
      wallet_default_address->setCbor(address);
      return outcome::success();
    };
    api->WalletSign = [=](auto address,
                          auto data) -> outcome::result<Signature> {
      if (!address.isKeyType()) {
        OUTCOME_TRY(context, tipsetContext({}));
        OUTCOME_TRYA(address, context.accountKey(address));
      }
      return key_store->sign(address, data);
    };
    api->WalletVerify =
        [=](auto address, auto data, auto signature) -> outcome::result<bool> {
      if (!address.isKeyType()) {
        OUTCOME_TRY(context, tipsetContext({}));
        OUTCOME_TRYA(address, context.accountKey(address));
      }
      return key_store->verify(address, data, signature);
    };
    /**
     * Payment channel methods are initialized with
     * PaymentChannelManager::makeApi(Api &api)
     */
    return api;
  }  // namespace fc::api
}  // namespace fc::api
