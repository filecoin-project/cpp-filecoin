/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/impl/chain_store_impl.hpp"

#include "primitives/cid/cid_of_cbor.hpp"

namespace fc::storage::blockchain {
  /** types */
  using primitives::address::Address;
  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;

  /** functions */
  using primitives::cid::getCidOfCbor;

  ChainStoreImpl::ChainStoreImpl(
      IpldPtr ipld,
      std::shared_ptr<WeightCalculator> weight_calculator,
      BlockHeader genesis,
      Tipset head)
      : data_store_{std::move(ipld)},
        weight_calculator_{std::move(weight_calculator)},
        heaviest_tipset_{std::move(head)},
        genesis_{std::move(genesis)} {
    OUTCOME_EXCEPT(weight,
                   weight_calculator_->calculateWeight(heaviest_tipset_));
    heaviest_weight_ = weight;
  }

  const Tipset &ChainStoreImpl::heaviestTipset() const {
    return heaviest_tipset_;
  }

  const BlockHeader &ChainStoreImpl::getGenesis() const {
    return genesis_;
  }

  outcome::result<void> ChainStoreImpl::addBlock(const BlockHeader &block) {
    OUTCOME_TRY(data_store_->setCbor(block));
    OUTCOME_TRY(tipset, expandTipset(block));
    return updateHeaviestTipset(tipset);
  }

  outcome::result<Tipset> ChainStoreImpl::expandTipset(
      const BlockHeader &block_header) {
    std::vector<BlockHeader> all_headers{block_header};

    if (tipsets_.find(block_header.height) == std::end(tipsets_)) {
      return Tipset::create(all_headers);
    }
    auto &&tipsets = tipsets_[block_header.height];

    std::set<Address> included_miners;
    included_miners.insert(block_header.miner);

    OUTCOME_TRY(block_cid, getCidOfCbor(block_header));
    for (auto &&cid : tipsets) {
      if (cid == block_cid) {
        continue;
      }

      OUTCOME_TRY(bh, data_store_->getCbor<BlockHeader>(cid));

      if (included_miners.count(bh.miner) > 0) {
        logger_->warn(
            "Have multiple blocks from miner {} at height {} in our tipset "
            "cache",
            bh.miner,
            bh.height);
        continue;
      }

      if (bh.parents == block_header.parents) {
        all_headers.push_back(bh);
        included_miners.insert(bh.miner);
      }
    }

    return Tipset::create(all_headers);
  }

  outcome::result<void> ChainStoreImpl::updateHeaviestTipset(
      const Tipset &tipset) {
    OUTCOME_TRY(weight, weight_calculator_->calculateWeight(tipset));

    OUTCOME_TRY(current_weight,
                weight_calculator_->calculateWeight(heaviest_tipset_));

    if (weight > current_weight) {
      return takeHeaviestTipset(tipset);
    }

    return outcome::success();
  }

  outcome::result<void> ChainStoreImpl::takeHeaviestTipset(
      const Tipset &tipset) {
    logger_->info("New heaviest tipset {} (height={})",
                  fmt::join(tipset.cids, ","),
                  tipset.height);

    OUTCOME_TRY(notifyHeadChange(heaviest_tipset_, tipset));
    heaviest_tipset_ = tipset;

    return outcome::success();
  }

  outcome::result<void> ChainStoreImpl::notifyHeadChange(const Tipset &current,
                                                         const Tipset &target) {
    OUTCOME_TRY(path, findChainPath(current, target));

    for (auto &revert_item : path.revert_chain) {
      head_change_signal_(
          HeadChange{.type = HeadChangeType::REVERT, .value = revert_item});
    }

    for (auto &apply_item : path.apply_chain) {
      head_change_signal_(
          HeadChange{.type = HeadChangeType::APPLY, .value = apply_item});
    }

    return outcome::success();
  }

  outcome::result<ChainPath> ChainStoreImpl::findChainPath(
      const Tipset &current, const Tipset &target) {
    ChainPath path{};
    auto l = current;
    auto r = target;
    while (l != r) {
      if (l.height == 0 && r.height == 0) {
        return ChainStoreError::kNoPath;
      }
      if (l.height > r.height) {
        path.revert_chain.emplace_back(l);
        OUTCOME_TRYA(l, l.loadParent(*data_store_));
      } else {
        path.apply_chain.emplace_front(r);
        OUTCOME_TRYA(r, r.loadParent(*data_store_));
      }
    }
    return path;
  }

}  // namespace fc::storage::blockchain

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::blockchain, ChainStoreError, e) {
  using fc::storage::blockchain::ChainStoreError;
  switch (e) {
    case ChainStoreError::kNoPath:
      return "no path";
  }
  return "ChainStoreError: unknown error";
}
