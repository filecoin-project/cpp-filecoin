/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/make.hpp"

#include "vm/actor/builtin/market/actor.hpp"
#include "vm/actor/builtin/miner/types.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::api {
  using vm::actor::kStorageMarketAddress;
  using vm::actor::builtin::miner::MinerActorState;
  using vm::state::StateTreeImpl;

  Api makeImpl(std::shared_ptr<ChainStore> chain_store,
               std::shared_ptr<WeightCalculator> weight_calculator,
               std::shared_ptr<IpfsDatastore> ipld,
               std::shared_ptr<KeyStore> key_store) {
    auto chain_randomness = chain_store->createRandomnessProvider();
    auto minerState = [&](auto &tipset_key,
                          auto &address) -> outcome::result<MinerActorState> {
      OUTCOME_TRY(tipset, chain_store->loadTipset(tipset_key));
      OUTCOME_TRY(
          actor, StateTreeImpl{ipld, tipset.getParentStateRoot()}.get(address));
      return ipld->getCbor<MinerActorState>(actor.head);
    };
    return {
        .ChainGetRandomness = {[&](auto &&tipset_key, auto round) {
          return chain_randomness->sampleRandomness(tipset_key.cids, round);
        }},
        .ChainHead = {[&]() { return chain_store->heaviestTipset(); }},
        // TODO(turuslan): FIL-165 implement method
        .ChainNotify = {},
        // TODO(turuslan): FIL-165 implement method
        .ChainReadObj = {},
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
        // TODO(turuslan): FIL-165 implement method
        .StateGetActor = {},
        // TODO(turuslan): FIL-165 implement method
        .StateMarketBalance = {},
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
