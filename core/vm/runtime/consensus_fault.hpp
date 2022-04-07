/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cbor_blake/ipld_version.hpp"
#include "primitives/block/rand.hpp"
#include "primitives/tipset/chain.hpp"
#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/runtime/env_context.hpp"
#include "vm/runtime/pricelist.hpp"
#include "vm/state/impl/state_tree_impl.hpp"
#include "vm/state/resolve_key.hpp"

namespace fc::vm::runtime {
  using actor::builtin::states::MinerActorStatePtr;
  using actor::builtin::types::miner::kChainFinality;
  using primitives::block::BlockHeader;
  using state::StateTreeImpl;

  template <typename T>
  inline bool has(const std::vector<T> &xs, const T &x) {
    return std::find(xs.begin(), xs.end(), x) != xs.end();
  }

  inline bool isNearOrange(ChainEpoch epoch) {
    return epoch > kUpgradeOrangeHeight - kChainFinality
           && epoch < kUpgradeOrangeHeight + kChainFinality;
  }

  struct ReadGasIpld : Ipld {
    const IpldPtr &ipld;
    const Pricelist &pricelist;
    mutable GasAmount gas{};

    ReadGasIpld(const IpldPtr &ipld, const Pricelist &pricelist)
        : ipld{ipld}, pricelist{pricelist} {}

    outcome::result<bool> contains(const CID &key) const override {
      return ERROR_TEXT("ReadGasIpld.contains");
    }
    outcome::result<void> set(const CID &key, BytesCow &&value) override {
      return ERROR_TEXT("ReadGasIpld.set");
    }
    outcome::result<Value> get(const CID &key) const override {
      gas += pricelist.onIpldGet();
      return ipld->get(key);
    }
  };

  /*
   * Verifies that two block headers provide proof of a consensus fault:
   * - both headers mined by the same actor
   * - headers are different
   * - first header is of the same or lower epoch as the second
   * - at least one of the headers appears in the current chain at or after
   *   epoch `earliest`
   * - the headers provide evidence of a fault (see the spec for the different
   *   fault types). The parameters are all serialized block headers. The third
   *   "extra" parameter is consulted only for the "parent grinding fault", in
   *   which case it must be the sibling of h1 (same parent tipset) and one of
   *   the blocks in the parent of h2 (i.e. h2's grandparent). Returns error
   *   if the headers don't prove a fault.
   */
  inline outcome::result<ConsensusFault> consensusFault(
      GasAmount &gas,
      const EnvironmentContext &envx,
      const TsBranchPtr &ts_branch,
      ChainEpoch epoch,
      const CID &base_state,
      BytesIn block1,
      BytesIn block2,
      BytesIn extra) {
    const auto kNoFault{ERROR_TEXT("consensusFault")};
    if (block1 == block2) {
      return kNoFault;
    }
    OUTCOME_TRY(blockA, codec::cbor::decode<BlockHeader>(block1));
    OUTCOME_TRY(blockB, codec::cbor::decode<BlockHeader>(block2));
    ConsensusFault fault;
    if (!blockA.miner.isId()) {
      return kNoFault;
    }
    fault.target = blockA.miner.getId();
    fault.epoch = blockB.height;
    boost::optional<ConsensusFaultType> type;
    if (isNearOrange(blockA.height) || isNearOrange(blockB.height)
        || blockA.miner != blockB.miner || blockA.height > blockB.height) {
      return kNoFault;
    }
    if (blockA.height == blockB.height) {
      type = ConsensusFaultType::DoubleForkMining;
    } else if (blockA.parents == blockB.parents) {
      type = ConsensusFaultType::TimeOffsetMining;
    }
    if (!extra.empty()) {
      OUTCOME_TRY(blockC, codec::cbor::decode<BlockHeader>(extra));
      if (blockA.parents == blockC.parents && blockA.height == blockC.height
          && has(blockB.parents, CbCid::hash(extra))
          && !has(blockB.parents, CbCid::hash(block1))) {
        type = ConsensusFaultType::ParentGrinding;
      }
    }
    if (!type) {
      return kNoFault;
    }
    const Pricelist pricelist{epoch};
    const auto ipld{withVersion(envx.ipld, epoch)};
    const auto gas_ipld{std::make_shared<ReadGasIpld>(ipld, pricelist)};
    gas_ipld->actor_version = ipld->actor_version;
    const auto BOOST_OUTCOME_TRY_UNIQUE_NAME{
        gsl::finally([&] { gas += gas_ipld->gas; })};
    const auto verify{[&](const BlockHeader &block) -> outcome::result<void> {
      if (version::getNetworkVersion(epoch) >= NetworkVersion::kVersion7
          && static_cast<ChainEpoch>(block.height) < epoch - kChainFinality) {
        return kNoFault;
      }
      std::shared_lock ts_lock{*envx.ts_branches_mutex};
      OUTCOME_TRY(
          it,
          find(ts_branch, std::min(epoch, ts_branch->chain.rbegin()->first)));
      OUTCOME_TRYA(it, getLookbackTipSetForRound(it, block.height));
      OUTCOME_TRYA(it, find(ts_branch, it.second->first + 1, false));
      OUTCOME_TRY(child_ts, envx.ts_load->lazyLoad(it.second->second));
      ts_lock.unlock();

      const StateTreeImpl lookback_tree{ipld, child_ts->getParentStateRoot()};

      OUTCOME_TRY(actor, lookback_tree.get(block.miner));

      OUTCOME_TRY(state, getCbor<MinerActorStatePtr>(gas_ipld, actor.head));
      OUTCOME_TRY(miner_info, state->getInfo());

      const StateTreeImpl tree{ipld, base_state};
      OUTCOME_TRY(key, resolveKey(tree, gas_ipld, miner_info->worker));
      OUTCOME_TRY(valid, checkBlockSignature(block, key));
      if (!valid) {
        return kNoFault;
      }
      return outcome::success();
    }};
    OUTCOME_TRY(verify(blockA));
    OUTCOME_TRY(verify(blockB));
    fault.type = *type;
    return fault;
  }
}  // namespace fc::vm::runtime
