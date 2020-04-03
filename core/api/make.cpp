/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/make.hpp"

#include "vm/actor/builtin/market/actor.hpp"
#include "vm/actor/builtin/miner/types.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::api {
  using vm::actor::kStorageMarketAddress;
  using vm::actor::builtin::miner::MinerActorState;
  using vm::interpreter::InterpreterImpl;
  using vm::state::StateTreeImpl;
  using MarketActorState = vm::actor::builtin::market::State;
  using crypto::signature::BlsSignature;
  using primitives::block::MsgMeta;
  using storage::amt::Amt;

  struct TipsetContext {
    Tipset tipset;
    StateTreeImpl state_tree;
    boost::optional<CID> receipts;

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
        OUTCOME_TRY(result, InterpreterImpl{}.interpret(ipld, tipset, nullptr));
        context.state_tree = {ipld, result.state_root};
        context.receipts = result.message_receipts;
      }
      return context;
    };
    return {
        .ChainGetRandomness = {[&](auto &&tipset_key, auto round) {
          return chain_randomness->sampleRandomness(tipset_key.cids, round);
        }},
        .ChainHead = {[&]() { return chain_store->heaviestTipset(); }},
        // TODO(turuslan): FIL-165 implement method
        .ChainNotify = {},
        .ChainReadObj = {[&](const auto &cid) { return ipld->get(cid); }},
        .ChainTipSetWight = {[&](auto tipset_key)
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

          OUTCOME_TRY(miner_state, context.minerState(miner));
          OUTCOME_TRY(worker_id,
                      context.state_tree.lookupId(miner_state.info.worker));
          OUTCOME_TRY(state_root, context.state_tree.flush());

          BlockMsg block;
          block.header.miner = miner;
          block.header.parents = parent.cids;
          block.header.ticket = ticket;
          block.header.height = height;
          block.header.timestamp = timestamp;
          block.header.epost_proof = proof;
          block.header.parent_state_root = state_root;
          block.header.parent_message_receipts = *context.receipts;
          block.header.fork_signaling = 0;

          std::vector<BlsSignature> bls_sigs;
          Amt amt_bls{ipld}, amt_secp{ipld};
          uint64_t i_bls{0}, i_secp{0};
          for (auto &message : messages) {
            if (message.signature.isBls()) {
              OUTCOME_TRY(message_cid, ipld->setCbor(message.message));
              bls_sigs.emplace_back(
                  boost::get<BlsSignature>(message.signature));
              block.bls_messages.emplace_back(std::move(message_cid));
              OUTCOME_TRY(amt_bls.setCbor(++i_bls, message_cid));
            } else {
              OUTCOME_TRY(message_cid, ipld->setCbor(message));
              block.secp_messages.emplace_back(std::move(message_cid));
              OUTCOME_TRY(amt_secp.setCbor(++i_secp, message_cid));
            }
          }

          OUTCOME_TRY(amt_bls.flush());
          OUTCOME_TRY(amt_secp.flush());
          OUTCOME_TRY(msg_meta,
                      ipld->setCbor(MsgMeta{amt_bls.cid(), amt_secp.cid()}));
          block.header.messages = msg_meta;

          OUTCOME_TRY(bls_aggregate,
                      bls_provider->aggregateSignatures(bls_sigs));
          block.header.bls_aggregate = bls_aggregate;

          OUTCOME_TRY(parent_weight,
                      weight_calculator->calculateWeight(context.tipset));
          block.header.parent_weight = parent_weight;

          OUTCOME_TRY(block_signable, codec::cbor::encode(block.header));
          OUTCOME_TRY(block_sig, key_store->sign(worker_id, block_signable));
          block.header.block_sig = block_sig;

          return block;
        }},
        // TODO(turuslan): FIL-165 implement method
        .MpoolPending = {},
        // TODO(turuslan): FIL-165 implement method
        .MpoolPushMessage = {},
        // TODO(turuslan): FIL-165 implement method
        .PaychVoucherAdd = {},
        // TODO(turuslan): FIL-165 implement method
        .StateCall = {},
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
        // TODO(turuslan): FIL-165 implement method
        .StateMinerPower = {},
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
