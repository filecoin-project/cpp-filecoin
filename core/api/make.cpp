/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/make.hpp"

#include <boost/algorithm/string.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "blockchain/production/block_producer.hpp"
#include "storage/hamt/hamt.hpp"
#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/actor/builtin/init/init_actor.hpp"
#include "vm/actor/builtin/market/actor.hpp"
#include "vm/actor/builtin/miner/types.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/runtime/env.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::api {
  using primitives::block::BlockHeader;
  using vm::actor::kInitAddress;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::kStoragePowerAddress;
  using vm::actor::builtin::account::AccountActorState;
  using vm::actor::builtin::init::InitActorState;
  using vm::actor::builtin::miner::MinerActorState;
  using vm::actor::builtin::storage_power::StoragePowerActorState;
  using vm::interpreter::InterpreterImpl;
  using InterpreterResult = vm::interpreter::Result;
  using vm::state::StateTreeImpl;
  using MarketActorState = vm::actor::builtin::market::State;
  using crypto::randomness::RandomnessProvider;
  using crypto::signature::BlsSignature;
  using libp2p::peer::PeerId;
  using primitives::block::MsgMeta;
  using vm::isVMExitCode;
  using vm::normalizeVMExitCode;
  using vm::VMExitCode;
  using vm::actor::InvokerImpl;
  using vm::runtime::Env;

  struct TipsetContext {
    Tipset tipset;
    StateTreeImpl state_tree;
    boost::optional<InterpreterResult> interpreted;

    template <typename T, bool load>
    outcome::result<T> actorState(const Address &address) {
      OUTCOME_TRY(actor, state_tree.get(address));
      OUTCOME_TRY(state, state_tree.getStore()->getCbor<T>(actor.head));
      if constexpr (load) {
        state.load(state_tree.getStore());
      }
      return std::move(state);
    }

    auto marketState() {
      return actorState<MarketActorState, true>(kStorageMarketAddress);
    }

    auto minerState(const Address &address) {
      return actorState<MinerActorState, true>(address);
    }

    auto powerState() {
      return actorState<StoragePowerActorState, true>(kStoragePowerAddress);
    }

    auto initState() {
      return actorState<InitActorState, false>(kInitAddress);
    }

    outcome::result<Address> accountKey(const Address &id) {
      // TODO(turuslan): error if not account
      OUTCOME_TRY(state, actorState<AccountActorState, false>(id));
      return state.address;
    }
  };

  Api makeImpl(std::shared_ptr<ChainStore> chain_store,
               std::shared_ptr<WeightCalculator> weight_calculator,
               std::shared_ptr<Ipld> ipld,
               std::shared_ptr<BlsProvider> bls_provider,
               std::shared_ptr<KeyStore> key_store,
               Logger logger) {
    auto chain_randomness = chain_store->createRandomnessProvider();
    auto tipsetContext = [=](const TipsetKey &tipset_key,
                             bool interpret =
                                 false) -> outcome::result<TipsetContext> {
      Tipset tipset;
      if (tipset_key.cids.empty()) {
        OUTCOME_TRYA(tipset, chain_store->heaviestTipset());
      } else {
        OUTCOME_TRYA(tipset, chain_store->loadTipset(tipset_key));
      }
      TipsetContext context{tipset, {ipld, tipset.getParentStateRoot()}, {}};
      if (interpret) {
        OUTCOME_TRY(result, InterpreterImpl{}.interpret(ipld, tipset));
        context.state_tree = {ipld, result.state_root};
        context.interpreted = result;
      }
      return context;
    };
    return {
        .AuthNew = {[](auto) {
          return Buffer{1, 2, 3};
        }},
        .ChainGetBlock = {[=](auto &block_cid) {
          return ipld->getCbor<BlockHeader>(block_cid);
        }},
        .ChainGetBlockMessages = {[=](auto &block_cid)
                                      -> outcome::result<BlockMessages> {
          BlockMessages messages;
          OUTCOME_TRY(block, ipld->getCbor<BlockHeader>(block_cid));
          OUTCOME_TRY(meta, ipld->getCbor<MsgMeta>(block.messages));
          meta.load(ipld);
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
        // TODO(turuslan): FIL-165 implement method
        .ChainGetGenesis = {},
        .ChainGetNode = {[=](auto &path) -> outcome::result<IpldObject> {
          std::vector<std::string> parts;
          boost::split(parts, path, [](auto c) { return c == '/'; });
          if (parts.size() < 3 || !parts[0].empty() || parts[1] != "ipfs") {
            return TodoError::ERROR;
          }
          OUTCOME_TRY(root, CID::fromString(parts[2]));
          return getNode(ipld, root, gsl::make_span(parts).subspan(3));
        }},
        .ChainGetMessage = {[=](auto &cid) -> outcome::result<UnsignedMessage> {
          auto res = chain_store->getCbor<SignedMessage>(cid);
          if (!res.has_error()) {
            return res.value().message;
          }

          return chain_store->getCbor<UnsignedMessage>(cid);
        }},
        .ChainGetParentMessages =
            {[=](auto &block_cid) -> outcome::result<std::vector<CidMessage>> {
              std::vector<CidMessage> messages;
              OUTCOME_TRY(block, ipld->getCbor<BlockHeader>(block_cid));
              for (auto &parent_cid : block.parents) {
                OUTCOME_TRY(parent, ipld->getCbor<BlockHeader>(parent_cid));
                OUTCOME_TRY(meta, ipld->getCbor<MsgMeta>(parent.messages));
                meta.load(ipld);
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
              return adt::Array<MessageReceipt>{block.parent_message_receipts}
                  .load(ipld)
                  .values();
            }},
        .ChainGetRandomness = {[=](auto &tipset_key, auto round) {
          return chain_randomness->sampleRandomness(tipset_key.cids, round);
        }},
        .ChainGetTipSet = {[=](auto &tipset_key) {
          return chain_store->loadTipset(tipset_key);
        }},
        .ChainGetTipSetByHeight = {[=](auto height2, auto &tipset_key)
                                       -> outcome::result<Tipset> {
          // TODO(turuslan): use height index from chain store
          // TODO(turuslan): return genesis if height is zero
          auto height = static_cast<uint64_t>(height2);
          OUTCOME_TRY(tipset,
                      tipset_key.cids.empty()
                          ? chain_store->heaviestTipset()
                          : chain_store->loadTipset(tipset_key));
          if (tipset.height < height) {
            return TodoError::ERROR;
          }
          while (tipset.height > height) {
            OUTCOME_TRY(parent, chain_store->loadParent(tipset));
            if (parent.height < height) {
              break;
            }
            tipset = std::move(parent);
          }
          return std::move(tipset);
        }},
        .ChainHead = {[=]() { return chain_store->heaviestTipset(); }},
        .ChainNotify = {[=]() {
          auto channel = std::make_shared<Channel<std::vector<HeadChange>>>();
          auto cnn = std::make_shared<ChainStore::connection_t>();
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
          return weight_calculator->calculateWeight(tipset);
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
        .MarketEnsureAvailable = {},
        .MinerCreateBlock = {[=](auto &t) -> outcome::result<BlockMsg> {
          OUTCOME_TRY(context, tipsetContext(t.parents, true));
          OUTCOME_TRY(miner_state, context.minerState(t.miner));
          OUTCOME_TRY(block,
                      blockchain::production::generate(ipld, std::move(t)));

          OUTCOME_TRY(block_signable, codec::cbor::encode(block.header));
          OUTCOME_TRY(worker_key, context.accountKey(miner_state.info.worker));
          OUTCOME_TRY(block_sig, key_store->sign(worker_key, block_signable));
          block.header.block_sig = block_sig;

          BlockMsg block2;
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
        .MinerGetBaseInfo = {[=](auto &miner, auto &tipset_key)
                                 -> outcome::result<MiningBaseInfo> {
          OUTCOME_TRY(context, tipsetContext(tipset_key, true));
          OUTCOME_TRY(state, context.minerState(miner));
          OUTCOME_TRY(power_state, context.powerState());
          MiningBaseInfo info;
          OUTCOME_TRY(claim, power_state.claims.get(miner));
          info.miner_power = claim.power;
          info.network_power = power_state.total_network_power;
          OUTCOME_TRY(state.proving_set.visit([&](auto i, auto s) {
            info.sectors.push_back({s, i});
            return outcome::success();
          }));
          OUTCOME_TRYA(info.worker, context.accountKey(state.info.worker));
          info.sector_size = state.info.sector_size;
          return info;
        }},
        // TODO(turuslan): FIL-165 implement method
        .MpoolPending = {},
        // TODO(turuslan): FIL-165 implement method
        .MpoolPushMessage = {},
        // TODO(turuslan): FIL-165 implement method
        .MpoolSub = {},
        // TODO(turuslan): FIL-165 implement method
        .NetAddrsListen = {},
        // TODO(turuslan): FIL-165 implement method
        .PaychVoucherAdd = {},
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
          // TODO(turuslan): FIL-146 randomness from tipset
          std::shared_ptr<RandomnessProvider> randomness;
          auto env = std::make_shared<Env>(
              randomness,
              std::make_shared<StateTreeImpl>(context.state_tree),
              std::make_shared<InvokerImpl>(),
              static_cast<ChainEpoch>(context.tipset.height));
          InvocResult result;
          result.message = message;
          auto maybe_result = env->applyImplicitMessage(message);
          if (maybe_result) {
            result.receipt = {VMExitCode::Ok, maybe_result.value(), 0};
          } else {
            if (isVMExitCode(maybe_result.error())) {
              auto ret_code =
                  normalizeVMExitCode(VMExitCode{maybe_result.error().value()});
              BOOST_ASSERT_MSG(ret_code,
                               "c++ actor code returned unknown error");
              result.receipt = {*ret_code, {}, 0};
            } else {
              return maybe_result.error();
            }
          }
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

          while (static_cast<int64_t>(context.tipset.height) >= to_height) {
            std::set<CID> visited_cid;

            auto isDuplicateMessage = [&](const CID &cid) -> bool {
              return !visited_cid.insert(cid).second;
            };

            for (const BlockHeader &block : context.tipset.blks) {
              OUTCOME_TRY(meta, ipld->getCbor<MsgMeta>(block.messages));
              meta.load(ipld);
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

            if (context.tipset.height == 0) break;

            OUTCOME_TRY(parent_tipset_key, context.tipset.getParents());
            OUTCOME_TRY(parent_context, tipsetContext(parent_tipset_key));

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
        // TODO(turuslan): FIL-165 implement method
        .StateGetReceipt = {},
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
          adt::Map<Actor, adt::AddressKeyer> actors(root);
          actors.load(ipld);

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
            OUTCOME_TRY(deal_state, state.getState(deal_id));
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
          OUTCOME_TRY(deal_state, state.getState(deal_id));
          return StorageDeal{deal, deal_state};
        }},
        .StateMinerElectionPeriodStart = {[=](auto address, auto tipset_key)
                                              -> outcome::result<ChainEpoch> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          return state.post_state.proving_period_start;
        }},
        .StateMinerFaults = {[=](auto address, auto tipset_key)
                                 -> outcome::result<RleBitset> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          return state.fault_set;
        }},
        .StateMinerInfo = {[=](auto &address,
                               auto &tipset_key) -> outcome::result<MinerInfo> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(miner_state, context.minerState(address));
          return miner_state.info;
        }},
        .StateMinerPostState = {[=](auto &address, auto &tipset_key)
                                    -> outcome::result<PoStState> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(miner_state, context.minerState(address));
          return miner_state.post_state;
        }},
        .StateMinerPower = {[=](auto &address, auto &tipset_key)
                                -> outcome::result<MinerPower> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));

          OUTCOME_TRY(power_state, context.powerState());
          StoragePower miner_power = 0;
          OUTCOME_TRY(claim, power_state.claims.tryGet(address));
          if (claim) {
            miner_power = claim->power;
          }

          OUTCOME_TRY(miner_state, context.minerState(address));
          if (miner_state.post_state.hasFailedPost()) {
            miner_power = 0;
          }

          return MinerPower{
              miner_power,
              power_state.total_network_power,
          };
        }},
        .StateMinerProvingSet =
            {[=](auto address, auto tipset_key)
                 -> outcome::result<std::vector<ChainSectorInfo>> {
              OUTCOME_TRY(context, tipsetContext(tipset_key));
              OUTCOME_TRY(state, context.minerState(address));
              std::vector<ChainSectorInfo> sectors;
              OUTCOME_TRY(state.proving_set.visit([&](auto id, auto &info) {
                sectors.emplace_back(ChainSectorInfo{info, id});
                return outcome::success();
              }));
              return sectors;
            }},
        .StateMinerSectors =
            {[=](auto address, auto filter, auto filter_out, auto tipset_key)
                 -> outcome::result<std::vector<ChainSectorInfo>> {
              OUTCOME_TRY(context, tipsetContext(tipset_key));
              OUTCOME_TRY(state, context.minerState(address));

              std::vector<ChainSectorInfo> res = {};

              auto visitor =
                  [&](uint64_t i,
                      const SectorOnChainInfo &info) -> outcome::result<void> {
                if (filter != nullptr) {
                  // TODO(artyom-yurin): implement filter
                };

                ChainSectorInfo sector_info{
                    .info = info,
                    .id = i,
                };

                res.push_back(sector_info);
                return outcome::success();
              };

              OUTCOME_TRY(state.sectors.visit(visitor));

              return res;
            }},
        .StateMinerSectorSize = {[=](auto address, auto tipset_key)
                                     -> outcome::result<SectorSize> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          return state.info.sector_size;
        }},
        .StateMinerWorker = {[=](auto address,
                                 auto tipset_key) -> outcome::result<Address> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          return state.info.worker;
        }},
        .StateNetworkName = {[=]() -> outcome::result<std::string> {
          OUTCOME_TRY(context, tipsetContext(chain_store->genesisTipsetKey()));
          OUTCOME_TRY(state, context.initState());
          return state.network_name;
        }},
        // TODO(turuslan): FIL-165 implement method
        .StateWaitMsg = {},
        .SyncSubmitBlock = {[=](auto block) -> outcome::result<void> {
          // TODO(turuslan): chain store must validate blocks before adding
          MsgMeta meta;
          meta.load(ipld);
          for (auto &cid : block.bls_messages) {
            OUTCOME_TRY(meta.bls_messages.append(cid));
          }
          for (auto &cid : block.secp_messages) {
            OUTCOME_TRY(meta.secp_messages.append(cid));
          }
          OUTCOME_TRY(meta.flush());
          OUTCOME_TRY(messages, ipld->setCbor(meta));
          if (block.header.messages != messages) {
            return TodoError::ERROR;
          }
          OUTCOME_TRY(chain_store->addBlock(block.header));
          return outcome::success();
        }},
        .Version = {[]() {
          return VersionResult{"fuhon", 0x000200, 5};
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
        .WalletSign = {[=](auto address,
                           auto data) -> outcome::result<Signature> {
          if (!address.isKeyType()) {
            OUTCOME_TRY(context, tipsetContext({}));
            OUTCOME_TRYA(address, context.accountKey(address));
          }
          return key_store->sign(address, data);
        }},
    };
  }
}  // namespace fc::api
