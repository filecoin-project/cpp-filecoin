/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/make.hpp"

#include "blockchain/production/impl/block_producer_impl.hpp"
#include "vm/actor/builtin/market/actor.hpp"
#include "vm/actor/builtin/miner/types.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/runtime/env.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::api {
  using primitives::block::BlockHeader;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::kStoragePowerAddress;
  using vm::actor::builtin::miner::MinerActorState;
  using vm::actor::builtin::storage_power::StoragePowerActorState;
  using vm::interpreter::InterpreterImpl;
  using InterpreterResult = vm::interpreter::Result;
  using vm::state::StateTreeImpl;
  using MarketActorState = vm::actor::builtin::market::State;
  using blockchain::production::BlockProducerImpl;
  using crypto::randomness::RandomnessProvider;
  using crypto::signature::BlsSignature;
  using primitives::block::MsgMeta;
  using storage::amt::Amt;
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
  };

  Api makeImpl(std::shared_ptr<ChainStore> chain_store,
               std::shared_ptr<WeightCalculator> weight_calculator,
               std::shared_ptr<IpfsDatastore> ipld,
               std::shared_ptr<BlsProvider> bls_provider,
               std::shared_ptr<KeyStore> key_store) {
    auto chain_randomness = chain_store->createRandomnessProvider();
    auto tipsetContext = [&](auto &tipset_key,
                             bool interpret =
                                 false) -> outcome::result<TipsetContext> {
      OUTCOME_TRY(tipset, chain_store->loadTipset(tipset_key));
      TipsetContext context{tipset, {ipld, tipset.getParentStateRoot()}, {}};
      if (interpret) {
        OUTCOME_TRY(result, InterpreterImpl{}.interpret(ipld, tipset));
        context.state_tree = {ipld, result.state_root};
        context.interpreted = result;
      }
      return context;
    };
    return {
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
            {[&](auto &block_cid)
                 -> outcome::result<std::vector<MessageReceipt>> {
              OUTCOME_TRY(block, ipld->getCbor<BlockHeader>(block_cid));
              return adt::Array<MessageReceipt>{block.parent_message_receipts}
                  .load(ipld)
                  .values();
            }},
        .ChainGetRandomness = {[&](auto &tipset_key, auto round) {
          return chain_randomness->sampleRandomness(tipset_key.cids, round);
        }},
        .ChainGetTipSet = {[&](auto &tipset_key) {
          return chain_store->loadTipset(tipset_key);
        }},
        .ChainGetTipSetByHeight = {[&](auto height2, auto &tipset_key)
                                       -> outcome::result<Tipset> {
          // TODO(turuslan): use height index from chain store
          // TODO(turuslan): error if height is negative or above starting
          // TODO(turuslan): return genesis if height is zero
          auto height = static_cast<uint64_t>(height2);
          OUTCOME_TRY(tipset,
                      tipset_key.cids.empty()
                          ? chain_store->heaviestTipset()
                          : chain_store->loadTipset(tipset_key));
          while (tipset.height > height) {
            OUTCOME_TRY(parent, chain_store->loadParent(tipset));
            if (parent.height < height) {
              break;
            }
            tipset = std::move(parent);
          }
          return std::move(tipset);
        }},
        .ChainHead = {[&]() { return chain_store->heaviestTipset(); }},
        // TODO(turuslan): FIL-165 implement method
        .ChainNotify = {},
        .ChainReadObj = {[&](const auto &cid) { return ipld->get(cid); }},
        .ChainTipSetWeight = {[&](auto &tipset_key)
                                  -> outcome::result<TipsetWeight> {
          OUTCOME_TRY(tipset, chain_store->loadTipset(tipset_key));
          return weight_calculator->calculateWeight(tipset);
        }},
        // TODO(turuslan): FIL-165 implement method
        .MarketEnsureAvailable = {},
        .MinerCreateBlock = {[&](auto &miner,
                                 auto &parent,
                                 auto &ticket,
                                 auto &proof,
                                 auto &messages,
                                 auto height,
                                 auto timestamp) -> outcome::result<BlockMsg> {
          OUTCOME_TRY(context, tipsetContext(parent, true));
          BlockProducerImpl producer{
              ipld,
              nullptr,
              nullptr,
              nullptr,
              weight_calculator,
              bls_provider,
              std::make_shared<InterpreterImpl>(),
          };
          OUTCOME_TRY(block,
                      producer.generate(miner,
                                        context.tipset,
                                        *context.interpreted,
                                        proof,
                                        ticket,
                                        messages,
                                        height,
                                        timestamp));

          OUTCOME_TRY(miner_state, context.minerState(miner));
          OUTCOME_TRY(worker_id,
                      context.state_tree.lookupId(miner_state.info.worker));
          OUTCOME_TRY(block_signable, codec::cbor::encode(block.header));
          OUTCOME_TRY(block_sig, key_store->sign(worker_id, block_signable));
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
        // TODO(turuslan): FIL-165 implement method
        .MpoolPending = {},
        // TODO(turuslan): FIL-165 implement method
        .MpoolPushMessage = {},
        // TODO(turuslan): FIL-165 implement method
        .PaychVoucherAdd = {},
        .StateCall = {[&](auto &message,
                          auto &tipset_key) -> outcome::result<InvocResult> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          // TODO(turuslan): FIL-146 randomness from tipset
          std::shared_ptr<RandomnessProvider> randomness;
          Env env{randomness,
                  std::make_shared<StateTreeImpl>(context.state_tree),
                  std::make_shared<InvokerImpl>(),
                  static_cast<ChainEpoch>(context.tipset.height)};
          InvocResult result;
          result.message = message;
          auto maybe_result = env.applyImplicitMessage(message);
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
        .StateGetActor = {[&](auto &address,
                              auto &tipset_key) -> outcome::result<Actor> {
          OUTCOME_TRY(context, tipsetContext(tipset_key, true));
          return context.state_tree.get(address);
        }},
        .StateMarketBalance =
            {[&](auto &address, auto &tipset_key)
                 -> outcome::result<StorageParticipantBalance> {
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
              return StorageParticipantBalance{*locked, *escrow - *locked};
            }},
        .StateMarketDeals = {[&](auto &tipset_key)
                                 -> outcome::result<MarketDealMap> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.marketState());
          MarketDealMap map;
          OUTCOME_TRY(state.proposals.visit([&](auto deal_id, auto &deal)
                                                -> outcome::result<void> {
            OUTCOME_TRY(deal_state, state.states.get(deal_id));
            map.emplace(std::to_string(deal_id), MarketDeal{deal, deal_state});
            return outcome::success();
          }));
          return map;
        }},
        .StateMarketStorageDeal = {[&](auto deal_id, auto &tipset_key)
                                       -> outcome::result<MarketDeal> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.marketState());
          OUTCOME_TRY(deal, state.proposals.get(deal_id));
          OUTCOME_TRY(deal_state, state.states.get(deal_id));
          return MarketDeal{deal, deal_state};
        }},
        .StateMinerElectionPeriodStart = {[&](auto address, auto tipset_key)
                                              -> outcome::result<ChainEpoch> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          return state.post_state.proving_period_start;
        }},
        .StateMinerFaults = {[&](auto address, auto tipset_key)
                                 -> outcome::result<RleBitset> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          return state.fault_set;
        }},
        .StateMinerPower = {[&](auto &address, auto &tipset_key)
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
            {[&](auto address, auto tipset_key)
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
        .StateMinerSectorSize = {[&](auto address, auto tipset_key)
                                     -> outcome::result<SectorSize> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          return state.info.sector_size;
        }},
        .StateMinerWorker = {[&](auto address,
                                 auto tipset_key) -> outcome::result<Address> {
          OUTCOME_TRY(context, tipsetContext(tipset_key));
          OUTCOME_TRY(state, context.minerState(address));
          return state.info.worker;
        }},
        // TODO(turuslan): FIL-165 implement method
        .StateWaitMsg = {},
        // TODO(turuslan): FIL-165 implement method
        .SyncSubmitBlock = {},
        .WalletSign = {[&](auto address, auto data) {
          return key_store->sign(address, data);
        }},
    };
  }
}  // namespace fc::api
