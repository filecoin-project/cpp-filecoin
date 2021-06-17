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
#include "common/logger.hpp"
#include "const.hpp"
#include "drand/beaconizer.hpp"
#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "node/pubsub_gate.hpp"
#include "primitives/tipset/chain.hpp"
#include "proofs/impl/proof_engine_impl.hpp"
#include "storage/chain/receipt_loader.hpp"
#include "storage/hamt/hamt.hpp"
#include "vm/actor/builtin/states/state_provider.hpp"
#include "vm/actor/builtin/states/verified_registry_actor_state.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"
#include "vm/actor/builtin/types/miner/types.hpp"
#include "vm/actor/builtin/types/storage_power/policy.hpp"
#include "vm/actor/builtin/v0/market/market_actor.hpp"
#include "vm/interpreter/interpreter.hpp"
#include "vm/message/impl/message_signer_impl.hpp"
#include "vm/message/message.hpp"
#include "vm/runtime/env.hpp"
#include "vm/runtime/impl/tipset_randomness.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

#define MOVE(x)  \
  x {            \
    std::move(x) \
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
  using vm::actor::builtin::states::StateProvider;
  using vm::actor::builtin::states::VerifiedRegistryActorStatePtr;
  using vm::actor::builtin::types::market::DealState;
  using vm::actor::builtin::types::storage_power::kConsensusMinerMinPower;
  using vm::message::kDefaultGasLimit;
  using vm::message::kDefaultGasPrice;
  using vm::runtime::Env;
  using vm::state::StateTreeImpl;
  using vm::version::getNetworkVersion;

  const static Logger logger = common::createLogger("Full node API");

  // TODO: reuse for block validation
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
                             CbT<std::vector<BeaconEntry>> cb) {
    auto max{schedule.maxRound(epoch)};
    if (max == prev) {
      return cb(outcome::success());
    }
    auto round{prev == 0 ? max : prev + 1};
    auto async{std::make_shared<AsyncAll<BeaconEntry>>(max - round + 1,
                                                       std::move(cb))};
    for (auto i{0u}; round <= max; ++round, ++i) {
      beaconizer.entry(round, async->on(i));
    }
  }

  template <typename T, typename F>
  auto waitCb(F &&f) {
    return [f{std::forward<F>(f)}](auto &&...args) {
      auto channel{std::make_shared<Channel<outcome::result<T>>>()};
      f(std::forward<decltype(args)>(args)..., [channel](auto &&_r) {
        channel->write(std::forward<decltype(_r)>(_r));
      });
      return Wait{channel};
    };
  }

  struct TipsetContext {
    TipsetCPtr tipset;
    StateTreeImpl state_tree;
    boost::optional<InterpreterResult> interpreted;

    auto marketState() -> outcome::result<MarketActorStatePtr> {
      const StateProvider provider(state_tree.getStore());
      OUTCOME_TRY(actor, state_tree.get(kStorageMarketAddress));
      OUTCOME_TRY(state, provider.getMarketActorState(actor));
      return std::move(state);
    }

    auto minerState(const Address &address)
        -> outcome::result<MinerActorStatePtr> {
      const StateProvider provider(state_tree.getStore());
      OUTCOME_TRY(actor, state_tree.get(address));
      OUTCOME_TRY(state, provider.getMinerActorState(actor));
      return std::move(state);
    }

    auto powerState() -> outcome::result<PowerActorStatePtr> {
      const StateProvider provider(state_tree.getStore());
      OUTCOME_TRY(actor, state_tree.get(kStoragePowerAddress));
      OUTCOME_TRY(state, provider.getPowerActorState(actor));
      return std::move(state);
    }

    auto initState() -> outcome::result<InitActorStatePtr> {
      const StateProvider provider(state_tree.getStore());
      OUTCOME_TRY(actor, state_tree.get(kInitAddress));
      OUTCOME_TRY(state, provider.getInitActorState(actor));
      return std::move(state);
    }

    auto verifiedRegistryState()
        -> outcome::result<VerifiedRegistryActorStatePtr> {
      const StateProvider provider(state_tree.getStore());
      OUTCOME_TRY(actor, state_tree.get(kVerifiedRegistryAddress));
      return provider.getVerifiedRegistryActorState(actor);
    }

    outcome::result<Address> accountKey(const Address &id) {
      const StateProvider provider(state_tree.getStore());
      OUTCOME_TRY(actor, state_tree.get(id));
      OUTCOME_TRY(state, provider.getAccountActorState(actor));
      return state->address;
    }
  };

  outcome::result<std::vector<SectorInfo>> getSectorsForWinningPoSt(
      const Address &miner,
      MinerActorStatePtr state,
      const Randomness &post_rand,
      IpldPtr ipld) {
    std::vector<SectorInfo> sectors;
    RleBitset sectors_bitset;
    OUTCOME_TRY(deadlines, state->deadlines.get());
    for (auto &deadline_cid : deadlines.due) {
      OUTCOME_TRY(deadline, state->getDeadline(ipld, deadline_cid));
      OUTCOME_TRY(deadline.partitions.visit([&](auto, auto &part) {
        for (auto sector : part.sectors) {
          if (!part.faults.has(sector)) {
            sectors_bitset.insert(sector);
          }
        }
        return outcome::success();
      }));
    }
    if (!sectors_bitset.empty()) {
      OUTCOME_TRY(minfo, state->getInfo(ipld));
      OUTCOME_TRY(win_type,
                  primitives::sector::getRegisteredWinningPoStProof(
                      minfo.seal_proof_type));
      static auto proofs{std::make_shared<proofs::ProofEngineImpl>()};
      OUTCOME_TRY(
          indices,
          proofs->generateWinningPoStSectorChallenge(
              win_type, miner.getId(), post_rand, sectors_bitset.size()));
      std::vector<uint64_t> sector_ids{sectors_bitset.begin(),
                                       sectors_bitset.end()};
      for (auto &i : indices) {
        OUTCOME_TRY(sector, state->sectors.get(sector_ids[i]));
        sectors.push_back(
            {minfo.seal_proof_type, sector.sector, sector.sealed_cid});
      }
    }
    return sectors;
  }

  std::shared_ptr<FullNodeApi> makeImpl(
      std::shared_ptr<ChainStore> chain_store,
      const std::string &network_name,
      std::shared_ptr<WeightCalculator> weight_calculator,
      const EnvironmentContext &env_context,
      TsBranchPtr ts_main,
      std::shared_ptr<MessagePool> mpool,
      std::shared_ptr<MsgWaiter> msg_waiter,
      std::shared_ptr<Beaconizer> beaconizer,
      std::shared_ptr<DrandSchedule> drand_schedule,
      std::shared_ptr<PubSubGate> pubsub,
      std::shared_ptr<KeyStore> key_store,
      std::shared_ptr<Discovery> market_discovery,
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
      TipsetContext context{tipset, {ipld, tipset->getParentStateRoot()}, {}};
      if (interpret) {
        OUTCOME_TRY(result, interpreter_cache->get(tipset->key));
        context.state_tree = {ipld, result.state_root};
        context.interpreted = result;
      }
      return context;
    };

    auto api = std::make_shared<FullNodeApi>();

    api->AuthNew = {[](auto) { return Buffer{1, 2, 3}; }};
    api->BeaconGetEntry = waitCb<BeaconEntry>([=](auto epoch, auto &&cb) {
      return beaconizer->entry(drand_schedule->maxRound(epoch), cb);
    });
    api->ChainGetBlock = {
        [=](auto &block_cid) { return ipld->getCbor<BlockHeader>(block_cid); }};
    api->ChainGetBlockMessages = {
        [=](auto &block_cid) -> outcome::result<BlockMessages> {
          BlockMessages messages;
          OUTCOME_TRY(block, ipld->getCbor<BlockHeader>(block_cid));
          OUTCOME_TRY(meta, ipld->getCbor<MsgMeta>(block.messages));
          OUTCOME_TRY(meta.bls_messages.visit(
              [&](auto, auto &cid) -> outcome::result<void> {
                OUTCOME_TRY(message, ipld->getCbor<UnsignedMessage>(cid));
                messages.bls.push_back(std::move(message));
                messages.cids.push_back(cid);
                return outcome::success();
              }));
          OUTCOME_TRY(meta.secp_messages.visit(
              [&](auto, auto &cid) -> outcome::result<void> {
                OUTCOME_TRY(message, ipld->getCbor<SignedMessage>(cid));
                messages.secp.push_back(std::move(message));
                messages.cids.push_back(cid);
                return outcome::success();
              }));
          return messages;
        }};
    api->ChainGetGenesis = {[=]() -> outcome::result<TipsetCPtr> {
      return ts_load->lazyLoad(ts_main->chain.begin()->second);
    }};
    api->ChainGetNode = {[=](auto &path) -> outcome::result<IpldObject> {
      std::vector<std::string> parts;
      boost::split(parts, path, [](auto c) { return c == '/'; });
      if (parts.size() < 3 || !parts[0].empty() || parts[1] != "ipfs") {
        return ERROR_TEXT("ChainGetNode: invalid path");
      }
      OUTCOME_TRY(root, CID::fromString(parts[2]));
      return getNode(ipld, root, gsl::make_span(parts).subspan(3));
    }};
    api->ChainGetMessage = {[=](auto &cid) -> outcome::result<UnsignedMessage> {
      auto res = ipld->getCbor<SignedMessage>(cid);
      if (!res.has_error()) {
        return res.value().message;
      }

      return ipld->getCbor<UnsignedMessage>(cid);
    }};
    api->ChainGetParentMessages = {
        [=](auto &block_cid) -> outcome::result<std::vector<CidMessage>> {
          std::vector<CidMessage> messages;
          OUTCOME_TRY(block, ipld->getCbor<BlockHeader>(block_cid));
          for (auto &parent_cid : block.parents) {
            OUTCOME_TRY(parent, ipld->getCbor<BlockHeader>(CID{parent_cid}));
            OUTCOME_TRY(meta, ipld->getCbor<MsgMeta>(parent.messages));
            OUTCOME_TRY(meta.bls_messages.visit(
                [&](auto, auto &cid) -> outcome::result<void> {
                  OUTCOME_TRY(message, ipld->getCbor<UnsignedMessage>(cid));
                  messages.push_back({cid, std::move(message)});
                  return outcome::success();
                }));
            OUTCOME_TRY(meta.secp_messages.visit(
                [&](auto, auto &cid) -> outcome::result<void> {
                  OUTCOME_TRY(message, ipld->getCbor<SignedMessage>(cid));
                  messages.push_back({cid, std::move(message.message)});
                  return outcome::success();
                }));
          }
          return messages;
        }};
    api->ChainGetParentReceipts = {
        [=](auto &block_cid) -> outcome::result<std::vector<MessageReceipt>> {
          OUTCOME_TRY(block, ipld->getCbor<BlockHeader>(block_cid));
          return adt::Array<MessageReceipt>{block.parent_message_receipts, ipld}
              .values();
        }};
    api->ChainGetRandomnessFromBeacon = {
        [=](auto &tipset_key, auto tag, auto epoch, auto &entropy)
            -> outcome::result<Randomness> {
          std::shared_lock ts_lock{*env_context.ts_branches_mutex};
          OUTCOME_TRY(ts_branch, TsBranch::make(ts_load, tipset_key, ts_main));
          return env_context.randomness->getRandomnessFromBeacon(
              ts_branch, tag, epoch, entropy);
        }};
    api->ChainGetRandomnessFromTickets = {
        [=](auto &tipset_key, auto tag, auto epoch, auto &entropy)
            -> outcome::result<Randomness> {
          std::shared_lock ts_lock{*env_context.ts_branches_mutex};
          OUTCOME_TRY(ts_branch, TsBranch::make(ts_load, tipset_key, ts_main));
          return env_context.randomness->getRandomnessFromTickets(
              ts_branch, tag, epoch, entropy);
        }};
    api->ChainGetTipSet = {
        [=](auto &tipset_key) { return ts_load->load(tipset_key); }};
    api->ChainGetTipSetByHeight = {
        [=](auto height, auto &tipset_key) -> outcome::result<TipsetCPtr> {
          std::shared_lock ts_lock{*env_context.ts_branches_mutex};
          OUTCOME_TRY(ts_branch, TsBranch::make(ts_load, tipset_key, ts_main));
          OUTCOME_TRY(it, find(ts_branch, height));
          return ts_load->lazyLoad(it.second->second);
        }};
    api->ChainHead = {[=]() { return chain_store->heaviestTipset(); }};
    api->ChainNotify = {[=]() {
      auto channel = std::make_shared<Channel<std::vector<HeadChange>>>();
      auto cnn = std::make_shared<connection_t>();
      *cnn = chain_store->subscribeHeadChanges([=](auto &change) {
        if (!channel->write({change})) {
          assert(cnn->connected());
          cnn->disconnect();
        }
      });
      return Chan{std::move(channel)};
    }};
    api->ChainReadObj = {[=](const auto &cid) { return ipld->get(cid); }};
    // TODO(turuslan): FIL-165 implement method
    api->ChainSetHead = {};
    api->ChainTipSetWeight = {
        [=](auto &tipset_key) -> outcome::result<TipsetWeight> {
          OUTCOME_TRY(tipset, ts_load->load(tipset_key));
          return weight_calculator->calculateWeight(*tipset);
        }};

    api->ClientFindData = waitCb<std::vector<QueryOffer>>(
        [=](auto &&root_cid, auto &&piece_cid, auto &&cb) {
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
              AsyncWaiter<RetrievalPeer, outcome::result<QueryResponse>>>(
              peers.size(), [=](auto all_calls) {
                std::vector<QueryOffer> result;
                for (const auto &[peer, maybe_response] : all_calls) {
                  if (maybe_response.has_error()) {
                    logger->error("Error when query peer {}",
                                  maybe_response.error().message());
                  } else {
                    auto response{maybe_response.value()};
                    std::string error_message;
                    switch (response.response_status) {
                      case markets::retrieval::QueryResponseStatus::
                          kQueryResponseAvailable:
                        break;
                      case markets::retrieval::QueryResponseStatus::
                          kQueryResponseUnavailable:
                        error_message =
                            "retrieval query offer was unavailable: "
                            + response.message;
                        break;
                      case markets::retrieval::QueryResponseStatus::
                          kQueryResponseError:
                        error_message = "retrieval query offer errored: "
                                        + response.message;
                        break;
                    }

                    result.emplace_back(QueryOffer{
                        error_message,
                        root_cid,
                        piece_cid,
                        response.item_size,
                        response.unseal_price
                            + response.min_price_per_byte * response.item_size,
                        response.unseal_price,
                        response.payment_interval,
                        response.interval_increase,
                        response.payment_address,
                        peer});
                  }
                }
                cb(result);
              });
          for (const auto &peer : peers) {
            OUTCOME_CB1(retrieval_market_client->query(
                peer, {root_cid, {piece_cid}}, waiter->on(peer)));
          }
        });

    // TODO(turuslan): FIL-165 implement method
    api->ClientHasLocal = {};
    // TODO(turuslan): FIL-165 implement method
    api->ClientImport = {};
    // TODO(turuslan): FIL-165 implement method
    api->ClientListImports = {};
    // TODO(turuslan): FIL-165 implement method
    api->ClientQueryAsk = {};

    /**
     * Initiates the retrieval deal of a file in retrieval market.
     */
    api->ClientRetrieve = waitCb<None>(
        [retrieval_market_client](auto &&order, auto &&file_ref, auto &&cb) {
          if (order.size == 0) {
            cb(ERROR_TEXT("Cannot make retrieval deal for zero bytes"));
          }
          auto price_per_byte = bigdiv(order.total, order.size);
          DealProposalParams params{
              .selector = kAllSelector,
              .piece = order.piece,
              .price_per_byte = price_per_byte,
              .payment_interval = order.payment_interval,
              .payment_interval_increase = order.payment_interval_increase,
              .unseal_price = order.unseal_price};
          OUTCOME_CB1(retrieval_market_client->retrieve(
              order.root,
              params,
              order.total,
              order.peer,
              order.client,
              order.miner,
              [cb{std::move(cb)}](outcome::result<void> res) {
                if (res.has_error()) {
                  logger->error("Error in ClientRetrieve {}",
                                res.error().message());
                  cb(res.error());
                } else {
                  logger->info("retrieval deal proposed");
                  cb(outcome::success());
                }
              }));
        });

    api->ClientStartDeal = {/* impemented in node/main.cpp */};

    api->GasEstimateFeeCap = {[=](auto &msg, auto max_blocks, auto &) {
      return mpool->estimateFeeCap(msg.gas_premium, max_blocks);
    }};
    api->GasEstimateGasPremium = {[=](auto max_blocks, auto &, auto, auto &) {
      return mpool->estimateGasPremium(max_blocks);
    }};
    api->GasEstimateMessageGas = {
        [=](auto msg, auto &spec, auto &) -> outcome::result<UnsignedMessage> {
          OUTCOME_TRY(mpool->estimate(
              msg, spec ? spec->max_fee : storage::mpool::kDefaultMaxFee));
          return msg;
        }};

    api->MarketReserveFunds = {[=](const Address &wallet,
                                   const Address &address,
                                   const TokenAmount &amount)
                                   -> outcome::result<boost::optional<CID>> {
      // TODO(a.chernyshov): method should use fund manager batch reserve and
      // release funds requests for market actor.
      vm::actor::builtin::v0::market::AddBalance::Params params{address};
      OUTCOME_TRY(encoded_params, codec::cbor::encode(params));
      UnsignedMessage unsigned_message{
          kStorageMarketAddress,
          wallet,
          {},
          amount,
          0,
          0,
          // TODO (a.chernyshov) there is v0 actor method number, but the actor
          // methods do not depend on version. Should be changed to general
          // method number when methods number are made general
          vm::actor::builtin::v0::market::AddBalance::Number,
          encoded_params};
      OUTCOME_TRY(signed_message,
                  api->MpoolPushMessage(unsigned_message, api::kPushNoSpec));
      return signed_message.getCid();
    }};

    api->MinerCreateBlock = {[=](auto &t) -> outcome::result<BlockWithCids> {
      OUTCOME_TRY(context, tipsetContext(t.parents, true));
      OUTCOME_TRY(miner_state, context.minerState(t.miner));
      OUTCOME_TRY(block,
                  blockchain::production::generate(
                      *interpreter_cache, ts_load, ipld, std::move(t)));

      OUTCOME_TRY(block_signable, codec::cbor::encode(block.header));
      OUTCOME_TRY(minfo, miner_state->getInfo(ipld));
      OUTCOME_TRY(worker_key, context.accountKey(minfo.worker));
      OUTCOME_TRY(block_sig, key_store->sign(worker_key, block_signable));
      block.header.block_sig = block_sig;

      BlockWithCids block2;
      block2.header = block.header;
      for (auto &msg : block.bls_messages) {
        OUTCOME_TRY(cid, ipld->setCbor(msg));
        block2.bls_messages.emplace_back(std::move(cid));
      }
      for (auto &msg : block.secp_messages) {
        OUTCOME_TRY(cid, ipld->setCbor(msg));
        block2.secp_messages.emplace_back(std::move(cid));
      }
      return block2;
    }};
    api->MinerGetBaseInfo = waitCb<boost::optional<MiningBaseInfo>>(
        [=](auto &&miner, auto epoch, auto &&tipset_key, auto &&cb) {
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
              [=, MOVE(cb), MOVE(context), MOVE(info)](auto _beacons) mutable {
                OUTCOME_CB(info.beacons, _beacons);
                TipsetContext lookback{
                    nullptr, {ipld, std::move(cached.state_root)}, {}};
                OUTCOME_CB(auto miner_state, lookback.minerState(miner));
                OUTCOME_CB(auto seed, codec::cbor::encode(miner));
                auto post_rand{crypto::randomness::drawRandomness(
                    info.beacon().data,
                    DomainSeparationTag::WinningPoStChallengeSeed,
                    epoch,
                    seed)};
                OUTCOME_CB(info.sectors,
                           getSectorsForWinningPoSt(
                               miner, miner_state, post_rand, ipld));
                if (info.sectors.empty()) {
                  return cb(boost::none);
                }
                OUTCOME_CB(auto power_state, lookback.powerState());
                OUTCOME_CB(auto claim, power_state->getClaim(miner));
                info.miner_power = claim.qa_power;
                info.network_power = power_state->total_qa_power;
                OUTCOME_CB(auto minfo, miner_state->getInfo(ipld));
                OUTCOME_CB(info.worker, context.accountKey(minfo.worker));
                info.sector_size = minfo.sector_size;
                info.has_min_power = minerHasMinPower(
                    claim.qa_power, power_state->num_miners_meeting_min_power);
                cb(std::move(info));
              });
        });
    api->MpoolPending = {
        [=](auto &tipset_key) -> outcome::result<std::vector<SignedMessage>> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          if (context.tipset->height()
              > chain_store->heaviestTipset()->height()) {
            return ERROR_TEXT("MpoolPending: tipset from future requested");
          }
          return mpool->pending();
        }};
    api->MpoolPushMessage = {
        [=](auto message, auto &spec) -> outcome::result<SignedMessage> {
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
                      vm::message::MessageSignerImpl{key_store}.sign(
                          message.from, message));
          OUTCOME_TRY(mpool->add(signed_message));
          return std::move(signed_message);
        }};
    api->MpoolSelect = {[=](auto &tsk, auto ticket_quality)
                            -> outcome::result<std::vector<SignedMessage>> {
      OUTCOME_TRY(ts, ts_load->load(tsk));
      return mpool->select(ts, ticket_quality);
    }};
    api->MpoolSub = {[=]() {
      auto channel{std::make_shared<Channel<MpoolUpdate>>()};
      auto cnn{std::make_shared<connection_t>()};
      *cnn = mpool->subscribe([=](auto &change) {
        if (!channel->write(change)) {
          assert(cnn->connected());
          cnn->disconnect();
        }
      });
      return Chan{std::move(channel)};
    }};
    api->StateAccountKey = {
        [=](auto &address, auto &tipset_key) -> outcome::result<Address> {
          if (address.isKeyType()) {
            return address;
          }
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          return context.accountKey(address);
        }};
    api->StateCall = {[=](auto &message,
                          auto &tipset_key) -> outcome::result<InvocResult> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));

      std::shared_lock ts_lock{*env_context.ts_branches_mutex};
      OUTCOME_TRY(ts_branch, TsBranch::make(ts_load, tipset_key, ts_main));
      ts_lock.unlock();

      auto env = std::make_shared<Env>(env_context, ts_branch, context.tipset);
      InvocResult result;
      result.message = message;
      OUTCOME_TRYA(result.receipt, env->applyImplicitMessage(message));
      return result;
    }};
    api->StateListMessages = {[=](auto &match, auto &tipset_key, auto to_height)
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
          OUTCOME_TRY(meta, ipld->getCbor<MsgMeta>(block.messages));
          OUTCOME_TRY(meta.bls_messages.visit(
              [&](auto, auto &cid) -> outcome::result<void> {
                OUTCOME_TRY(message, ipld->getCbor<UnsignedMessage>(cid));

                if (!isDuplicateMessage(cid) && matchFunc(message)) {
                  result.push_back(cid);
                }

                return outcome::success();
              }));
          OUTCOME_TRY(meta.secp_messages.visit(
              [&](auto, auto &cid) -> outcome::result<void> {
                OUTCOME_TRY(message, ipld->getCbor<SignedMessage>(cid));

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
    }};
    api->StateGetActor = {
        [=](auto &address, auto &tipset_key) -> outcome::result<Actor> {
          OUTCOME_TRY(context, tipsetContext(tipset_key, true));
          return context.state_tree.get(address);
        }};
    api->StateReadState = {
        [=](auto &actor, auto &tipset_key) -> outcome::result<ActorState> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          auto cid = actor.head;
          OUTCOME_TRY(raw, context.state_tree.getStore()->get(cid));
          return ActorState{
              .balance = actor.balance,
              .state = IpldObject{std::move(cid), std::move(raw)},
          };
        }};
    api->StateGetReceipt = {
        [=](auto &cid, auto &tipset_key) -> outcome::result<MessageReceipt> {
          auto receipt_loader =
              std::make_shared<storage::blockchain::ReceiptLoader>(ts_load,
                                                                   ipld);
          OUTCOME_TRY(
              result,
              receipt_loader->searchBackForMessageReceipt(cid, tipset_key, 0));
          if (result.has_value()) {
            return result->first;
          }
          return ERROR_TEXT("StateGetReceipt: no receipt");
        }};
    api->StateListMiners = {
        [=](auto &tipset_key) -> outcome::result<std::vector<Address>> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(power_state, context.powerState());
          return power_state->getClaimsKeys();
        }};
    api->StateListActors = {
        [=](auto &tipset_key) -> outcome::result<std::vector<Address>> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(root, context.state_tree.flush());
          adt::Map<Actor, adt::AddressKeyer> actors{root, ipld};

          return actors.keys();
        }};
    api->StateMarketBalance = {
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
        }};
    api->StateMarketDeals = {
        [=](auto &tipset_key) -> outcome::result<MarketDealMap> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.marketState());
          MarketDealMap map;
          OUTCOME_TRY(state->proposals.visit([&](auto deal_id, auto &deal)
                                                 -> outcome::result<void> {
            OUTCOME_TRY(deal_state, state->states.get(deal_id));
            map.emplace(std::to_string(deal_id), StorageDeal{deal, deal_state});
            return outcome::success();
          }));
          return map;
        }};
    api->StateLookupID = {
        [=](auto &address, auto &tipset_key) -> outcome::result<Address> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          return context.state_tree.lookupId(address);
        }};
    api->StateMarketStorageDeal = {
        [=](auto deal_id, auto &tipset_key) -> outcome::result<StorageDeal> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.marketState());
          OUTCOME_TRY(deal, state->proposals.get(deal_id));
          OUTCOME_TRY(deal_state, state->states.tryGet(deal_id));
          if (!deal_state) {
            deal_state = DealState{kChainEpochUndefined,
                                   kChainEpochUndefined,
                                   kChainEpochUndefined};
          }
          return StorageDeal{deal, *deal_state};
        }};
    api->StateMinerDeadlines = {
        [=](auto &address,
            auto &tipset_key) -> outcome::result<std::vector<Deadline>> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          OUTCOME_TRY(deadlines, state->deadlines.get());
          std::vector<Deadline> result;
          for (const auto &deadline_cid : deadlines.due) {
            OUTCOME_TRY(deadline, state->getDeadline(ipld, deadline_cid));
            result.push_back(Deadline{deadline.post_submissions});
          }
          return result;
        }};
    api->StateMinerFaults = {
        [=](auto address, auto tipset_key) -> outcome::result<RleBitset> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          OUTCOME_TRY(deadlines, state->deadlines.get());
          RleBitset faults;
          for (auto &deadline_cid : deadlines.due) {
            OUTCOME_TRY(deadline, state->getDeadline(ipld, deadline_cid));
            OUTCOME_TRY(deadline.partitions.visit([&](auto, auto &part) {
              faults += part.faults;
              return outcome::success();
            }));
          }
          return faults;
        }};
    api->StateMinerInfo = {
        [=](auto &address, auto &tipset_key) -> outcome::result<MinerInfo> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(miner_state, context.minerState(address));
          return miner_state->getInfo(ipld);
        }};
    api->StateMinerPartitions = {
        [=](auto &miner,
            auto _deadline,
            auto &tsk) -> outcome::result<std::vector<Partition>> {
          OUTCOME_TRY(context, tipsetContext(tsk));
          OUTCOME_TRY(state, context.minerState(miner));
          OUTCOME_TRY(deadlines, state->deadlines.get());
          OUTCOME_TRY(deadline,
                      state->getDeadline(ipld, deadlines.due[_deadline]));
          std::vector<Partition> parts;
          OUTCOME_TRY(deadline.partitions.visit([&](auto, auto &v) {
            parts.push_back({
                v.sectors,
                v.faults,
                v.recoveries,
                v.sectors - v.terminated,
                v.sectors - v.terminated - v.faults,
            });
            return outcome::success();
          }));
          return parts;
        }};
    api->StateMinerPower = {
        [=](auto &address, auto &tipset_key) -> outcome::result<MinerPower> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(power_state, context.powerState());
          OUTCOME_TRY(miner_power, power_state->getClaim(address));
          Claim total(power_state->total_raw_power,
                      power_state->total_qa_power);

          return MinerPower{miner_power, total};
        }};
    api->StateMinerProvingDeadline = {
        [=](auto &address, auto &tipset_key) -> outcome::result<DeadlineInfo> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          const auto deadline_info =
              state->deadlineInfo(context.tipset->height());
          return deadline_info.nextNotElapsed();
        }};
    api->StateMinerSectors = {
        [=](auto &address, auto &filter, auto &tipset_key)
            -> outcome::result<std::vector<SectorOnChainInfo>> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          std::vector<SectorOnChainInfo> sectors;
          OUTCOME_TRY(state->sectors.visit([&](auto id, auto &info) {
            if (!filter || filter->count(id)) {
              sectors.push_back(info);
            }
            return outcome::success();
          }));
          return sectors;
        }};
    api->StateNetworkName = {
        [=]() -> outcome::result<std::string> { return network_name; }};
    api->StateNetworkVersion =
        [=](auto &tipset_key) -> outcome::result<NetworkVersion> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      return getNetworkVersion(context.tipset->height());
    };
    // TODO(artyom-yurin): FIL-165 implement method
    api->StateMinerPreCommitDepositForPower = {};
    // TODO(artyom-yurin): FIL-165 implement method
    api->StateMinerInitialPledgeCollateral = {};

    api->GetProofType = [=](const Address &miner_address,
                            const TipsetKey &tipset_key)
        -> outcome::result<RegisteredSealProof> {
      OUTCOME_TRY(context, tipsetContext(tipset_key));
      OUTCOME_TRY(miner_state, context.minerState(miner_address));
      OUTCOME_TRY(miner_info, miner_state->getInfo(ipld));
      auto network_version = getNetworkVersion(context.tipset->height());
      return getPreferredSealProofTypeFromWindowPoStType(
          network_version, miner_info.window_post_proof_type);
    };

    // TODO(artyom-yurin): FIL-165 implement method
    api->StateSectorPreCommitInfo = {};
    api->StateSectorGetInfo = {
        [=](auto address, auto sector_number, auto tipset_key)
            -> outcome::result<boost::optional<SectorOnChainInfo>> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          return state->sectors.tryGet(sector_number);
        }};
    // TODO(artyom-yurin): FIL-165 implement method
    api->StateSectorPartition = {};

    api->StateVerifiedClientStatus = [=](const Address &address,
                                         const TipsetKey &tipset_key)
        -> outcome::result<boost::optional<StoragePower>> {
      OUTCOME_TRY(context, tipsetContext(tipset_key, true));
      OUTCOME_TRY(id, context.state_tree.lookupId(address));
      OUTCOME_TRY(state, context.verifiedRegistryState());
      return state->getVerifiedClientDataCap(id);
    };

    // TODO(artyom-yurin): FIL-165 implement method
    api->StateSearchMsg = {};
    api->StateWaitMsg =
        waitCb<MsgWait>([=](auto &&cid, auto &&confidence, auto &&cb) {
          // look for message on chain
          const auto receipt_loader =
              std::make_shared<storage::blockchain::ReceiptLoader>(ts_load,
                                                                   ipld);
          OUTCOME_CB(auto result,
                     receipt_loader->searchBackForMessageReceipt(
                         cid, chain_store->heaviestTipset()->getParents(), 0));
          if (result.has_value()) {
            OUTCOME_CB(auto ts, ts_load->load(result->second));
            cb(MsgWait{
                cid, result->first, result->second, (ChainEpoch)ts->height()});
            return;
          }

          // if message was not found, wait for it
          msg_waiter->wait(cid, [=, MOVE(cb)](auto &result) {
            OUTCOME_CB(auto ts, ts_load->load(result.second));
            cb(MsgWait{cid, result.first, ts->key, (ChainEpoch)ts->height()});
          });
        });
    api->SyncSubmitBlock = {[=](auto block) -> outcome::result<void> {
      // TODO(turuslan): chain store must validate blocks before adding
      MsgMeta meta;
      ipld->load(meta);
      for (auto &cid : block.bls_messages) {
        OUTCOME_TRY(meta.bls_messages.append(cid));
      }
      for (auto &cid : block.secp_messages) {
        OUTCOME_TRY(meta.secp_messages.append(cid));
      }
      OUTCOME_TRY(messages, ipld->setCbor(meta));
      if (block.header.messages != messages) {
        return ERROR_TEXT("SyncSubmitBlock: messages cid doesn't match");
      }
      OUTCOME_TRY(chain_store->addBlock(block.header));
      OUTCOME_TRY(pubsub->publish(block));
      return outcome::success();
    }};
    api->Version = {[]() {
      return VersionResult{"fuhon", makeApiVersion(2, 0, 0), 5};
    }};
    api->WalletBalance = {[=](auto &address) -> outcome::result<TokenAmount> {
      OUTCOME_TRY(context, tipsetContext({}));
      OUTCOME_TRY(actor, context.state_tree.get(address));
      return actor.balance;
    }};
    api->WalletDefaultAddress = {[=]() -> outcome::result<Address> {
      if (!wallet_default_address->has())
        return ERROR_TEXT("WalletDefaultAddress: default wallet is not set");
      return wallet_default_address->getCbor<Address>();
    }};
    api->WalletHas = {[=](auto address) -> outcome::result<bool> {
      if (!address.isKeyType()) {
        OUTCOME_TRY(context, tipsetContext({}));
        OUTCOME_TRYA(address, context.accountKey(address));
      }
      return key_store->has(address);
    }};
    api->WalletImport = {[=](auto &info) {
      return key_store->put(info.type == SignatureType::BLS, info.private_key);
    }};
    api->WalletSign = {
        [=](auto address, auto data) -> outcome::result<Signature> {
          if (!address.isKeyType()) {
            OUTCOME_TRY(context, tipsetContext({}));
            OUTCOME_TRYA(address, context.accountKey(address));
          }
          return key_store->sign(address, data);
        }};
    api->WalletVerify = {
        [=](auto address, auto data, auto signature) -> outcome::result<bool> {
          if (!address.isKeyType()) {
            OUTCOME_TRY(context, tipsetContext({}));
            OUTCOME_TRYA(address, context.accountKey(address));
          }
          return key_store->verify(address, data, signature);
        }};
    /**
     * Payment channel methods are initialized with
     * PaymentChannelManager::makeApi(Api &api)
     */
    return api;
  }  // namespace fc::api
}  // namespace fc::api
