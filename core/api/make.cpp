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

  Api makeImpl(std::shared_ptr<ChainStore> chain_store,
               std::shared_ptr<WeightCalculator> weight_calculator,
               std::shared_ptr<IpfsDatastore> ipld,
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
    auto marketState = [&](auto &tipset_key) -> outcome::result<MarketActorState> {
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
        // TODO(turuslan): FIL-165 implement method
        .MinerCreateBlock = {},
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
        // TODO(turuslan): FIL-165 implement method
        .StateMarketDeals = {},
        // TODO(turuslan): FIL-165 implement method
        .StateMarketStorageDeal = {},
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
        // TODO(turuslan): FIL-165 implement method
        .StateMinerProvingSet = {},
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
