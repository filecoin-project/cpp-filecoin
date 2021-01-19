/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/make.hpp"

#include <boost/algorithm/string.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "blockchain/production/block_producer.hpp"
#include "const.hpp"
#include "drand/beaconizer.hpp"
#include "node/pubsub.hpp"
#include "proofs/proofs.hpp"
#include "storage/hamt/hamt.hpp"
#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/builtin/v0/init/init_actor.hpp"
#include "vm/actor/builtin/v0/market/actor.hpp"
#include "vm/actor/builtin/v0/miner/types.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/message/impl/message_signer_impl.hpp"
#include "vm/runtime/env.hpp"
#include "vm/runtime/impl/tipset_randomness.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

#define MOVE(x)  \
  x {            \
    std::move(x) \
  }

namespace fc::api {
  using primitives::kChainEpochUndefined;
  using vm::actor::kInitAddress;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::kStoragePowerAddress;
  using vm::actor::builtin::v0::account::AccountActorState;
  using vm::actor::builtin::v0::init::InitActorState;
  using vm::actor::builtin::v0::market::DealState;
  using vm::actor::builtin::v0::miner::Deadline;
  using vm::actor::builtin::v0::miner::MinerActorState;
  using vm::actor::builtin::v0::storage_power::StoragePowerActorState;
  using InterpreterResult = vm::interpreter::Result;
  using crypto::randomness::DomainSeparationTag;
  using crypto::signature::BlsSignature;
  using libp2p::peer::PeerId;
  using primitives::block::MsgMeta;
  using vm::isVMExitCode;
  using vm::VMExitCode;
  using vm::actor::InvokerImpl;
  using vm::runtime::Env;
  using vm::runtime::TipsetRandomness;
  using vm::state::StateTreeImpl;
  using connection_t = boost::signals2::connection;
  using MarketActorState = vm::actor::builtin::v0::market::State;

  // TODO: reuse for block validation
  inline bool minerHasMinPower(const StoragePower &claim_qa,
                               size_t min_power_miners) {
    return min_power_miners < kConsensusMinerMinMiners
               ? !claim_qa.is_zero()
               : claim_qa > kConsensusMinerMinPower;
  }

  constexpr EpochDuration kWinningPoStSectorSetLookback{10};

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

    auto marketState() {
      return state_tree.state<MarketActorState>(kStorageMarketAddress);
    }

    auto minerState(const Address &address) {
      return state_tree.state<MinerActorState>(address);
    }

    auto powerState() {
      return state_tree.state<StoragePowerActorState>(kStoragePowerAddress);
    }

    auto initState() {
      return state_tree.state<InitActorState>(kInitAddress);
    }

    outcome::result<Address> accountKey(const Address &id) {
      // TODO(turuslan): error if not account
      OUTCOME_TRY(state, state_tree.state<AccountActorState>(id));
      return state.address;
    }
  };

  outcome::result<std::vector<SectorInfo>> getSectorsForWinningPoSt(
      IpldPtr ipld,
      const Address &miner,
      MinerActorState &state,
      const Randomness &post_rand) {
    std::vector<SectorInfo> sectors;
    RleBitset sectors_bitset;
    OUTCOME_TRY(deadlines, ipld->getCbor<Deadlines>(state.deadlines));
    for (auto &_deadline : deadlines.due) {
      OUTCOME_TRY(deadline, ipld->getCbor<Deadline>(_deadline));
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
      OUTCOME_TRY(minfo, ipld->getCbor<MinerInfo>(state.info));
      OUTCOME_TRY(win_type,
                  primitives::sector::getRegisteredWinningPoStProof(
                      minfo.seal_proof_type));
      OUTCOME_TRY(
          indices,
          proofs::Proofs::generateWinningPoStSectorChallenge(
              win_type, miner.getId(), post_rand, sectors_bitset.size()));
      std::vector<uint64_t> sector_ids{sectors_bitset.begin(),
                                       sectors_bitset.end()};
      for (auto &i : indices) {
        OUTCOME_TRY(sector, state.sectors.get(sector_ids[i]));
        sectors.push_back(
            {minfo.seal_proof_type, sector.sector, sector.sealed_cid});
      }
    }
    return sectors;
  }

  Api makeImpl(std::shared_ptr<ChainStore> chain_store,
               std::shared_ptr<WeightCalculator> weight_calculator,
               std::shared_ptr<Ipld> ipld,
               std::shared_ptr<Mpool> mpool,
               std::shared_ptr<Interpreter> interpreter,
               std::shared_ptr<MsgWaiter> msg_waiter,
               std::shared_ptr<Beaconizer> beaconizer,
               std::shared_ptr<DrandSchedule> drand_schedule,
               std::shared_ptr<PubSub> pubsub,
               std::shared_ptr<KeyStore> key_store) {
    auto tipsetContext = [=](const TipsetKey &tipset_key,
                             bool interpret =
                                 false) -> outcome::result<TipsetContext> {
      TipsetCPtr tipset;
      if (tipset_key.cids().empty()) {
        tipset = chain_store->heaviestTipset();
      } else {
        OUTCOME_TRYA(tipset, chain_store->loadTipset(tipset_key));
      }
      TipsetContext context{tipset, {ipld, tipset->getParentStateRoot()}, {}};
      if (interpret) {
        OUTCOME_TRY(result, interpreter->interpret(ipld, tipset));
        context.state_tree = {ipld, result.state_root};
        context.interpreted = result;
      }
      return context;
    };
    auto getLookbackTipSetForRound =
        [=](auto tipset, auto epoch) -> outcome::result<TipsetContext> {
      auto lookback{
          std::max(ChainEpoch{0}, epoch - kWinningPoStSectorSetLookback)};
      while (tipset->height() > static_cast<uint64_t>(lookback)) {
        OUTCOME_TRYA(tipset, tipset->loadParent(*ipld));
      }
      OUTCOME_TRY(result, interpreter->interpret(ipld, tipset));
      return TipsetContext{
          std::move(tipset), {ipld, std::move(result.state_root)}, {}};
    };
    return {
        .AuthNew = {[](auto) {
          return Buffer{1, 2, 3};
        }},
        .BeaconGetEntry = waitCb<BeaconEntry>([=](auto epoch, auto &&cb) {
          return beaconizer->entry(drand_schedule->maxRound(epoch), cb);
        }),
        .ChainGetBlock = {[=](auto &block_cid) {
          return ipld->getCbor<BlockHeader>(block_cid);
        }},
        .ChainGetBlockMessages = {[=](auto &block_cid)
                                      -> outcome::result<BlockMessages> {
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
        }},
        .ChainGetGenesis = {[=]() -> outcome::result<TipsetCPtr> {
          return chain_store->loadTipsetByHeight(0);
        }},
        .ChainGetNode = {[=](auto &path) -> outcome::result<IpldObject> {
          std::vector<std::string> parts;
          boost::split(parts, path, [](auto c) { return c == '/'; });
          if (parts.size() < 3 || !parts[0].empty() || parts[1] != "ipfs") {
            return TodoError::kError;
          }
          OUTCOME_TRY(root, CID::fromString(parts[2]));
          return getNode(ipld, root, gsl::make_span(parts).subspan(3));
        }},
        .ChainGetMessage = {[=](auto &cid) -> outcome::result<UnsignedMessage> {
          auto res = ipld->getCbor<SignedMessage>(cid);
          if (!res.has_error()) {
            return res.value().message;
          }

          return ipld->getCbor<UnsignedMessage>(cid);
        }},
        .ChainGetParentMessages =
            {[=](auto &block_cid) -> outcome::result<std::vector<CidMessage>> {
              std::vector<CidMessage> messages;
              OUTCOME_TRY(block, ipld->getCbor<BlockHeader>(block_cid));
              for (auto &parent_cid : block.parents) {
                OUTCOME_TRY(parent, ipld->getCbor<BlockHeader>(parent_cid));
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
            }},
        .ChainGetParentReceipts =
            {[=](auto &block_cid)
                 -> outcome::result<std::vector<MessageReceipt>> {
              OUTCOME_TRY(block, ipld->getCbor<BlockHeader>(block_cid));
              return adt::Array<MessageReceipt>{block.parent_message_receipts,
                                                ipld}
                  .values();
            }},
        .ChainGetRandomnessFromBeacon = {[=](auto &tipset_key,
                                             auto tag,
                                             auto epoch,
                                             auto &entropy)
                                             -> outcome::result<Randomness> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          return context.tipset->beaconRandomness(*ipld, tag, epoch, entropy);
        }},
        .ChainGetRandomnessFromTickets = {[=](auto &tipset_key,
                                              auto tag,
                                              auto epoch,
                                              auto &entropy)
                                              -> outcome::result<Randomness> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          return context.tipset->ticketRandomness(*ipld, tag, epoch, entropy);
        }},
        .ChainGetTipSet = {[=](auto &tipset_key) {
          return chain_store->loadTipset(tipset_key);
        }},
        .ChainGetTipSetByHeight = {[=](auto height2, auto &tipset_key)
                                       -> outcome::result<TipsetCPtr> {
          // TODO(turuslan): use height index from chain store
          // TODO(turuslan): return genesis if height is zero
          auto height = static_cast<uint64_t>(height2);
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          auto &tipset{context.tipset};
          if (tipset->height() < height) {
            return TodoError::kError;
          }
          while (tipset->height() > height) {
            OUTCOME_TRY(parent, tipset->loadParent(*ipld));
            if (parent->height() < height) {
              break;
            }
            tipset = std::move(parent);
          }
          return std::move(tipset);
        }},
        .ChainHead = {[=]() { return chain_store->heaviestTipset(); }},
        .ChainNotify = {[=]() {
          auto channel = std::make_shared<Channel<std::vector<HeadChange>>>();
          auto cnn = std::make_shared<connection_t>();
          *cnn = chain_store->subscribeHeadChanges([=](auto &change) {
            if (!channel->write({change})) {
              assert(cnn->connected());
              cnn->disconnect();
            }
          });
          return Chan{std::move(channel)};
        }},
        .ChainReadObj = {[=](const auto &cid) { return ipld->get(cid); }},
        // TODO(turuslan): FIL-165 implement method
        .ChainSetHead = {},
        .ChainTipSetWeight = {[=](auto &tipset_key)
                                  -> outcome::result<TipsetWeight> {
          OUTCOME_TRY(tipset, chain_store->loadTipset(tipset_key));
          return weight_calculator->calculateWeight(*tipset);
        }},
        // TODO(turuslan): FIL-165 implement method
        .ClientFindData = {},
        // TODO(turuslan): FIL-165 implement method
        .ClientHasLocal = {},
        // TODO(turuslan): FIL-165 implement method
        .ClientImport = {},
        // TODO(turuslan): FIL-165 implement method
        .ClientListImports = {},
        // TODO(turuslan): FIL-165 implement method
        .ClientQueryAsk = {},
        // TODO(turuslan): FIL-165 implement method
        .ClientRetrieve = {},
        // TODO(turuslan): FIL-165 implement method
        .ClientStartDeal = {},
        // TODO(turuslan): FIL-165 implement method
        .DealsImportData = {},
        // TODO(turuslan): FIL-165 implement method
        .GasEstimateMessageGas = {},
        // TODO(turuslan): FIL-165 implement method
        .MarketGetAsk = {},
        // TODO(turuslan): FIL-165 implement method
        .MarketGetRetrievalAsk = {},
        // TODO(turuslan): FIL-165 implement method
        .MarketReserveFunds = {},
        // TODO(turuslan): FIL-165 implement method
        .MarketSetAsk = {},
        // TODO(turuslan): FIL-165 implement method
        .MarketSetRetrievalAsk = {},
        .MinerCreateBlock = {[=](auto &t) -> outcome::result<BlockWithCids> {
          OUTCOME_TRY(context, tipsetContext(t.parents, true));
          OUTCOME_TRY(miner_state, context.minerState(t.miner));
          OUTCOME_TRY(block,
                      blockchain::production::generate(
                          *interpreter, ipld, std::move(t)));

          OUTCOME_TRY(block_signable, codec::cbor::encode(block.header));
          OUTCOME_TRY(minfo, ipld->getCbor<MinerInfo>(miner_state.info));
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
        }},
        .MinerGetBaseInfo = waitCb<boost::optional<MiningBaseInfo>>(
            [=](auto &&miner, auto epoch, auto &&tipset_key, auto &&cb) {
              OUTCOME_CB(auto context, tipsetContext(tipset_key, true));
              MiningBaseInfo info;
              OUTCOME_CB(info.prev_beacon, context.tipset->latestBeacon(*ipld));
              auto prev{info.prev_beacon.round};
              beaconEntriesForBlock(
                  *drand_schedule,
                  *beaconizer,
                  epoch,
                  prev,
                  [=, MOVE(cb), MOVE(context), MOVE(info)](
                      auto _beacons) mutable {
                    OUTCOME_CB(info.beacons, _beacons);
                    OUTCOME_CB(
                        auto lookback,
                        getLookbackTipSetForRound(context.tipset, epoch));
                    OUTCOME_CB(auto state, lookback.minerState(miner));
                    OUTCOME_CB(auto seed, codec::cbor::encode(miner));
                    auto post_rand{crypto::randomness::drawRandomness(
                        info.beacon().data,
                        DomainSeparationTag::WinningPoStChallengeSeed,
                        epoch,
                        seed)};
                    OUTCOME_CB(info.sectors,
                               getSectorsForWinningPoSt(
                                   ipld, miner, state, post_rand));
                    if (info.sectors.empty()) {
                      return cb(boost::none);
                    }
                    OUTCOME_CB(auto power_state, lookback.powerState());
                    OUTCOME_CB(auto claim, power_state.claims.get(miner));
                    info.miner_power = claim.qa_power;
                    info.network_power = power_state.total_qa_power;
                    OUTCOME_CB(auto minfo,
                               ipld->getCbor<MinerInfo>(state.info));
                    OUTCOME_CB(info.worker, context.accountKey(minfo.worker));
                    info.sector_size = minfo.sector_size;
                    info.has_min_power = minerHasMinPower(
                        claim.qa_power,
                        power_state.num_miners_meeting_min_power);
                    cb(std::move(info));
                  });
            }),
        .MpoolPending = {[=](auto &tipset_key)
                             -> outcome::result<std::vector<SignedMessage>> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          if (context.tipset->height()
              > chain_store->heaviestTipset()->height()) {
            // tipset from future requested
            return TodoError::kError;
          }
          return mpool->pending();
        }},
        .MpoolPushMessage = {[=](auto message,
                                 auto) -> outcome::result<SignedMessage> {
          OUTCOME_TRY(context, tipsetContext({}));
          if (message.from.isId()) {
            OUTCOME_TRYA(message.from,
                         vm::runtime::resolveKey(
                             context.state_tree, ipld, message.from, false));
          }
          OUTCOME_TRY(mpool->estimate(message));
          OUTCOME_TRYA(message.nonce, mpool->nonce(message.from));
          OUTCOME_TRY(signed_message,
                      vm::message::MessageSignerImpl{key_store}.sign(
                          message.from, message));
          OUTCOME_TRY(mpool->add(signed_message));
          return std::move(signed_message);
        }},
        .MpoolSelect = {[=](auto &, auto) {
          // TODO: implement
          return mpool->pending();
        }},
        .MpoolSub = {[=]() {
          auto channel{std::make_shared<Channel<MpoolUpdate>>()};
          auto cnn{std::make_shared<connection_t>()};
          *cnn = mpool->subscribe([=](auto &change) {
            if (!channel->write(change)) {
              assert(cnn->connected());
              cnn->disconnect();
            }
          });
          return Chan{std::move(channel)};
        }},
        // TODO(turuslan): FIL-165 implement method
        .NetAddrsListen = {},
        .PledgeSector = {},
        .StateAccountKey = {[=](auto &address,
                                auto &tipset_key) -> outcome::result<Address> {
          if (address.isKeyType()) {
            return address;
          }
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          return context.accountKey(address);
        }},
        .StateCall = {[=](auto &message,
                          auto &tipset_key) -> outcome::result<InvocResult> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          auto randomness = std::make_shared<TipsetRandomness>(ipld);
          auto env = std::make_shared<Env>(std::make_shared<InvokerImpl>(),
                                           randomness,
                                           ipld,
                                           context.tipset);
          InvocResult result;
          result.message = message;
          OUTCOME_TRYA(result.receipt, env->applyImplicitMessage(message));
          return result;
        }},
        .StateListMessages = {[=](auto &match, auto &tipset_key, auto to_height)
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

                    if (!isDuplicateMessage(cid)
                        && matchFunc(message.message)) {
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
        }},
        .StateGetActor = {[=](auto &address,
                              auto &tipset_key) -> outcome::result<Actor> {
          OUTCOME_TRY(context, tipsetContext(tipset_key, true));
          return context.state_tree.get(address);
        }},
        .StateReadState = {[=](auto &actor, auto &tipset_key)
                               -> outcome::result<ActorState> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          auto cid = actor.head;
          OUTCOME_TRY(raw, context.state_tree.getStore()->get(cid));
          return ActorState{
              .balance = actor.balance,
              .state = IpldObject{std::move(cid), std::move(raw)},
          };
        }},
        .StateGetReceipt = {[=](auto &cid, auto &tipset_key)
                                -> outcome::result<MessageReceipt> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          auto result{msg_waiter->results.find(cid)};
          if (result != msg_waiter->results.end()) {
            OUTCOME_TRY(ts, Tipset::load(*ipld, result->second.second.cids()));
            if (context.tipset->height() <= ts->height()) {
              return result->second.first;
            }
          }
          return TodoError::kError;
        }},
        .StateListMiners = {[=](auto &tipset_key)
                                -> outcome::result<std::vector<Address>> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(power_state, context.powerState());
          return power_state.claims.keys();
        }},
        .StateListActors = {[=](auto &tipset_key)
                                -> outcome::result<std::vector<Address>> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(root, context.state_tree.flush());
          adt::Map<Actor, adt::AddressKeyer> actors{root, ipld};

          return actors.keys();
        }},
        .StateMarketBalance = {[=](auto &address, auto &tipset_key)
                                   -> outcome::result<MarketBalance> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.marketState());
          OUTCOME_TRY(id_address, context.state_tree.lookupId(address));
          OUTCOME_TRY(escrow, state.escrow_table.tryGet(id_address));
          OUTCOME_TRY(locked, state.locked_table.tryGet(id_address));
          if (!escrow) {
            escrow = 0;
          }
          if (!locked) {
            locked = 0;
          }
          return MarketBalance{*escrow, *locked};
        }},
        .StateMarketDeals = {[=](auto &tipset_key)
                                 -> outcome::result<MarketDealMap> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.marketState());
          MarketDealMap map;
          OUTCOME_TRY(state.proposals.visit([&](auto deal_id, auto &deal)
                                                -> outcome::result<void> {
            OUTCOME_TRY(deal_state, state.states.get(deal_id));
            map.emplace(std::to_string(deal_id), StorageDeal{deal, deal_state});
            return outcome::success();
          }));
          return map;
        }},
        .StateLookupID = {[=](auto &address,
                              auto &tipset_key) -> outcome::result<Address> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          return context.state_tree.lookupId(address);
        }},
        .StateMarketStorageDeal = {[=](auto deal_id, auto &tipset_key)
                                       -> outcome::result<StorageDeal> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.marketState());
          OUTCOME_TRY(deal, state.proposals.get(deal_id));
          OUTCOME_TRY(deal_state, state.states.tryGet(deal_id));
          if (!deal_state) {
            deal_state = DealState{kChainEpochUndefined,
                                   kChainEpochUndefined,
                                   kChainEpochUndefined};
          }
          return StorageDeal{deal, *deal_state};
        }},
        .StateMinerDeadlines = {[=](auto &address, auto &tipset_key)
                                    -> outcome::result<Deadlines> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          return ipld->getCbor<Deadlines>(state.deadlines);
        }},
        .StateMinerFaults = {[=](auto address, auto tipset_key)
                                 -> outcome::result<RleBitset> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          OUTCOME_TRY(deadlines, ipld->getCbor<Deadlines>(state.deadlines));
          RleBitset faults;
          for (auto &_deadline : deadlines.due) {
            OUTCOME_TRY(deadline, ipld->getCbor<Deadline>(_deadline));
            OUTCOME_TRY(deadline.partitions.visit([&](auto, auto &part) {
              faults += part.faults;
              return outcome::success();
            }));
          }
          return faults;
        }},
        .StateMinerInfo = {[=](auto &address,
                               auto &tipset_key) -> outcome::result<MinerInfo> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(miner_state, context.minerState(address));
          return ipld->getCbor<MinerInfo>(miner_state.info);
        }},
        .StateMinerPartitions =
            {[=](auto &miner,
                 auto _deadline,
                 auto &tsk) -> outcome::result<std::vector<Partition>> {
              OUTCOME_TRY(context, tipsetContext(tsk));
              OUTCOME_TRY(state, context.minerState(miner));
              OUTCOME_TRY(deadlines, ipld->getCbor<Deadlines>(state.deadlines));
              OUTCOME_TRY(deadline,
                          ipld->getCbor<Deadline>(deadlines.due[_deadline]));
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
            }},
        .StateMinerPower = {[=](auto &address, auto &tipset_key)
                                -> outcome::result<MinerPower> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(power_state, context.powerState());
          OUTCOME_TRY(miner_power, power_state.claims.get(address));
          return MinerPower{
              miner_power,
              {power_state.total_raw_power, power_state.total_qa_power},
          };
        }},
        .StateMinerProvingDeadline = {[=](auto &address, auto &tipset_key)
                                          -> outcome::result<DeadlineInfo> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          const auto deadline_info =
              state.deadlineInfo(context.tipset->height());
          return deadline_info.nextNotElapsed();
        }},
        .StateMinerSectors =
            {[=](auto &address, auto &filter, auto &tipset_key)
                 -> outcome::result<std::vector<SectorOnChainInfo>> {
              OUTCOME_TRY(context, tipsetContext(tipset_key));
              OUTCOME_TRY(state, context.minerState(address));
              std::vector<SectorOnChainInfo> sectors;
              OUTCOME_TRY(state.sectors.visit([&](auto id, auto &info) {
                if (!filter || filter->count(id)) {
                  sectors.push_back(info);
                }
                return outcome::success();
              }));
              return sectors;
            }},
        .StateNetworkName = {[=]() -> outcome::result<std::string> {
          return chain_store->getNetworkName();
        }},
        .StateNetworkVersion =
            [=](auto &tipset_key) -> outcome::result<NetworkVersion> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          return vm::version::getNetworkVersion(context.tipset->height());
        },
        // TODO(artyom-yurin): FIL-165 implement method
        .StateMinerPreCommitDepositForPower = {},
        // TODO(artyom-yurin): FIL-165 implement method
        .StateMinerInitialPledgeCollateral = {},
        // TODO(artyom-yurin): FIL-165 implement method
        .StateSectorPreCommitInfo = {},
        .StateSectorGetInfo =
            {[=](auto address, auto sector_number, auto tipset_key)
                 -> outcome::result<boost::optional<SectorOnChainInfo>> {
              OUTCOME_TRY(context, tipsetContext(tipset_key));
              OUTCOME_TRY(state, context.minerState(address));
              return state.sectors.tryGet(sector_number);
            }},
        // TODO(artyom-yurin): FIL-165 implement method
        .StateSectorPartition = {},
        // TODO(artyom-yurin): FIL-165 implement method
        .StateSearchMsg = {},
        .StateWaitMsg = waitCb<MsgWait>([=](auto &&cid,
                                            auto &&confidence,
                                            auto &&cb) {
          msg_waiter->wait(cid, [=, MOVE(cb)](auto &result) {
            OUTCOME_CB(auto ts, chain_store->loadTipset(result.second));
            cb(MsgWait{cid, result.first, ts->key, (ChainEpoch)ts->height()});
          });
        }),
        .SyncSubmitBlock = {[=](auto block) -> outcome::result<void> {
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
            return TodoError::kError;
          }
          OUTCOME_TRY(chain_store->addBlock(block.header));
          OUTCOME_TRY(pubsub->publish(block));
          return outcome::success();
        }},
        .Version = {[]() {
          return VersionResult{"fuhon", 0x000C00, 5};
        }},
        .WalletBalance = {[=](auto &address) -> outcome::result<TokenAmount> {
          OUTCOME_TRY(context, tipsetContext({}));
          OUTCOME_TRY(actor, context.state_tree.get(address));
          return actor.balance;
        }},
        // TODO(turuslan): FIL-165 implement method
        .WalletDefaultAddress = {},
        .WalletHas = {[=](auto address) -> outcome::result<bool> {
          if (!address.isKeyType()) {
            OUTCOME_TRY(context, tipsetContext({}));
            OUTCOME_TRYA(address, context.accountKey(address));
          }
          return key_store->has(address);
        }},
        .WalletImport = {[=](auto &info) {
          return key_store->put(info.type == SignatureType::BLS,
                                info.private_key);
        }},
        .WalletSign = {[=](auto address,
                           auto data) -> outcome::result<Signature> {
          if (!address.isKeyType()) {
            OUTCOME_TRY(context, tipsetContext({}));
            OUTCOME_TRYA(address, context.accountKey(address));
          }
          return key_store->sign(address, data);
        }},
        .WalletVerify = {[=](auto address,
                             auto data,
                             auto signature) -> outcome::result<bool> {
          if (!address.isKeyType()) {
            OUTCOME_TRY(context, tipsetContext({}));
            OUTCOME_TRYA(address, context.accountKey(address));
          }
          return key_store->verify(address, data, signature);
        }},
        /**
         * Payment channel methods are initialized with
         * PaymentChannelManager::makeApi(Api &api)
         */
        .PaychAllocateLane = {},
        .PaychGet = {},
        .PaychVoucherAdd = {},
        .PaychVoucherCheckValid = {},
        .PaychVoucherCreate = {},
    };
  }
}  // namespace fc::api
