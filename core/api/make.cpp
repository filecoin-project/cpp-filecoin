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

  Api makeImpl(std::shared_ptr<ChainStore> chain_store,
               std::shared_ptr<WeightCalculator> weight_calculator,
               std::shared_ptr<IpfsDatastore> ipld,
               std::shared_ptr<BlsProvider> bls_provider,
               std::shared_ptr<KeyStore> key_store) {
    auto chain_randomness = chain_store->createRandomnessProvider();
    auto getActor = [&](auto &tipset_key,
                        auto &address,
                        bool use_parent_state =
                            true) -> outcome::result<Actor> {
      OUTCOME_TRY(tipset, chain_store->loadTipset(tipset_key));
      auto state_root = tipset.getParentStateRoot();
      if (!use_parent_state) {
        OUTCOME_TRY(result, InterpreterImpl{}.interpret(ipld, tipset, nullptr));
        state_root = result.state_root;
      }
      return StateTreeImpl{ipld, state_root}.get(address);
    };
    auto minerState = [&](auto &tipset_key,
                          auto &address) -> outcome::result<MinerActorState> {
      OUTCOME_TRY(actor, getActor(tipset_key, address));
      return ipld->getCbor<MinerActorState>(actor.head);
    };
    auto marketState =
        [&](auto &tipset_key) -> outcome::result<MarketActorState> {
      OUTCOME_TRY(actor, getActor(tipset_key, kStorageMarketAddress));
      OUTCOME_TRY(state, ipld->getCbor<MarketActorState>(actor.head));
      state.load(ipld);
      return std::move(state);
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
          OUTCOME_TRY(tipset, chain_store->loadTipset(parent));
          OUTCOME_TRY(interpreted,
                      InterpreterImpl{}.interpret(ipld, tipset, nullptr));

          StateTreeImpl state_tree{ipld, interpreted.state_root};
          OUTCOME_TRY(miner_actor, state_tree.get(miner));
          OUTCOME_TRY(miner_state,
                      ipld->getCbor<MinerActorState>(miner_actor.head));
          OUTCOME_TRY(worker_id, state_tree.lookupId(miner_state.info.worker));

          BlockMsg block;
          block.header.miner = miner;
          block.header.parents = parent.cids;
          block.header.ticket = ticket;
          block.header.height = height;
          block.header.timestamp = timestamp;
          block.header.epost_proof = proof;
          block.header.parent_state_root = interpreted.state_root;
          block.header.parent_message_receipts = interpreted.message_receipts;
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
                      weight_calculator->calculateWeight(tipset));
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
        .StateGetActor = {[&](auto &address, auto &tipset_key) {
          return getActor(tipset_key, address, false);
        }},
        .StateMarketBalance =
            {[&](auto &address, auto &tipset_key)
                 -> outcome::result<StorageParticipantBalance> {
              OUTCOME_TRY(state, marketState(tipset_key));
              OUTCOME_TRY(escrow, state.escrow_table.tryGet(address));
              OUTCOME_TRY(locked, state.locked_table.tryGet(address));
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
          OUTCOME_TRY(state, marketState(tipset_key));
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
          OUTCOME_TRY(state, marketState(tipset_key));
          OUTCOME_TRY(deal, state.proposals.get(deal_id));
          OUTCOME_TRY(deal_state, state.states.get(deal_id));
          return MarketDeal{deal, deal_state};
        }},
        .StateMinerElectionPeriodStart = {[&](auto address, auto tipset_key)
                                              -> outcome::result<ChainEpoch> {
          OUTCOME_TRY(state, minerState(tipset_key, address));
          return state.post_state.proving_period_start;
        }},
        .StateMinerFaults = {[&](auto address, auto tipset_key)
                                 -> outcome::result<RleBitset> {
          OUTCOME_TRY(state, minerState(tipset_key, address));
          return state.fault_set;
        }},
        // TODO(turuslan): FIL-165 implement method
        .StateMinerPower = {},
        .StateMinerProvingSet =
            {[&](auto address, auto tipset_key)
                 -> outcome::result<std::vector<ChainSectorInfo>> {
              OUTCOME_TRY(state, minerState(tipset_key, address));
              std::vector<ChainSectorInfo> sectors;
              OUTCOME_TRY(state.proving_set.visit([&](auto id, auto &info) {
                sectors.emplace_back(ChainSectorInfo{info, id});
                return outcome::success();
              }));
              return sectors;
            }},
        .StateMinerSectorSize = {[&](auto address, auto tipset_key)
                                     -> outcome::result<SectorSize> {
          OUTCOME_TRY(state, minerState(tipset_key, address));
          return state.info.sector_size;
        }},
        .StateMinerWorker = {[&](auto address,
                                 auto tipset_key) -> outcome::result<Address> {
          OUTCOME_TRY(state, minerState(tipset_key, address));
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
