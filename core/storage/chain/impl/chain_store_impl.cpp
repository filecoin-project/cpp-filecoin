/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/impl/chain_store_impl.hpp"

#include "common/outcome.hpp"
#include "crypto/randomness/impl/chain_randomness_provider_impl.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "primitives/cid/json_codec.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "storage/chain/datastore_key.hpp"
#include "storage/chain/impl/chain_data_store_impl.hpp"

namespace fc::storage::blockchain {
  /** types */
  using crypto::randomness::ChainRandomnessProvider;
  using crypto::randomness::ChainRandomnessProviderImpl;
  using primitives::address::Address;
  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;

  /** functions */
  using codec::json::decodeCidVector;
  using codec::json::encodeCidVector;
  using primitives::cid::getCidOfCbor;

  namespace {
    const DatastoreKey kChainHeadKey{DatastoreKey::makeFromString("head")};
    const DatastoreKey kGenesisKey{DatastoreKey::makeFromString("0")};
  }  // namespace

  ChainStoreImpl::ChainStoreImpl(
      std::shared_ptr<IpfsDatastore> data_store,
      std::shared_ptr<BlockValidator> block_validator,
      std::shared_ptr<WeightCalculator> weight_calculator)
      : data_store_{std::move(data_store)},
        block_validator_{std::move(block_validator)},
        weight_calculator_{std::move(weight_calculator)} {
    chain_data_store_ = std::make_shared<ChainDataStoreImpl>(data_store_);
    logger_ = common::createLogger("chain store");
  }

  outcome::result<std::shared_ptr<ChainStoreImpl>> ChainStoreImpl::create(
      std::shared_ptr<IpfsDatastore> data_store,
      std::shared_ptr<BlockValidator> block_validator,
      std::shared_ptr<WeightCalculator> weight_calculator) {
    ChainStoreImpl tmp(std::move(data_store),
                       std::move(block_validator),
                       std::move(weight_calculator));

    return std::make_shared<ChainStoreImpl>(std::move(tmp));
  }

  outcome::result<Tipset> ChainStoreImpl::loadTipset(
      const TipsetKey &key) const {
    // check cache first
    if (auto it = tipsets_cache_.find(key); it != tipsets_cache_.end()) {
      return it->second;
    }
    OUTCOME_TRY(tipset, Tipset::load(*data_store_, key.cids()));
    // save to cache
    tipsets_cache_[key] = tipset;

    return std::move(tipset);
  }

  outcome::result<void> ChainStoreImpl::initialize() {
    // load head tipset
    auto &&head_key = chain_data_store_->get(kChainHeadKey);
    if (head_key) {
      OUTCOME_TRY(cids, decodeCidVector(head_key.value()));
      OUTCOME_TRY(ts_key, TipsetKey::create(std::move(cids)));
      OUTCOME_TRY(tipset, loadTipset(ts_key));
      heaviest_tipset_.reset(tipset);
    } else {
      logger_->warn("no previous head tipset found, error = {}",
                    head_key.error().message());
    }

    // load genesis block
    auto genesis_key = chain_data_store_->get(kGenesisKey);
    if (genesis_key) {
      OUTCOME_TRY(genesis_cids, decodeCidVector(genesis_key.value()));
      BOOST_ASSERT_MSG(genesis_cids.size() == 1, "wrong genesis key format");
      OUTCOME_TRY(genesis_value, getCbor<BlockHeader>(genesis_cids[0]));
      genesis_.reset(genesis_value);
    } else {
      logger_->warn("no previous genesis block found, error = {}",
                    head_key.error().message());
    }

    return outcome::success();
  }

  outcome::result<Tipset> ChainStoreImpl::heaviestTipset() const {
    if (heaviest_tipset_.has_value()) {
      return *heaviest_tipset_;
    }
    return ChainStoreError::NO_HEAVIEST_TIPSET;
  }

  outcome::result<bool> ChainStoreImpl::containsTipset(
      const TipsetKey &key) const {
    if (tipsets_cache_.count(key) > 0) {
      return true;
    }

    OUTCOME_TRY(loadTipset(key));
    return true;
  }

  outcome::result<BlockHeader> ChainStoreImpl::getGenesis() const {
    if (genesis_.has_value()) {
      return *genesis_;
    }

    return ChainStoreError::NO_GENESIS_BLOCK;
  }

  outcome::result<void> ChainStoreImpl::writeGenesis(
      const BlockHeader &block_header) {
    OUTCOME_TRY(genesis_tipset, Tipset::create({block_header}));
    OUTCOME_TRY(
        buffer,
        encodeCidVector(std::vector<CID>{genesis_tipset.key.cids()[0]}));
    genesis_ = block_header;
    return chain_data_store_->set(kGenesisKey, buffer);
  }

  outcome::result<void> ChainStoreImpl::writeHead(const Tipset &tipset) {
    heaviest_tipset_.reset(tipset);
    OUTCOME_TRY(weight, weight_calculator_->calculateWeight(tipset));
    heaviest_weight_ = weight;
    OUTCOME_TRY(data, encodeCidVector(tipset.key.cids()));
    return chain_data_store_->set(kChainHeadKey, data);
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

      OUTCOME_TRY(bh, getCbor<BlockHeader>(cid));

      if (included_miners.count(bh.miner) > 0) {
        auto &&miner_address = encodeToString(bh.miner);
        logger_->warn(
            "Have multiple blocks from miner {} at height {} in our tipset "
            "cache",
            miner_address,
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

    if (!heaviest_tipset_.has_value()) {
      return takeHeaviestTipset(tipset);
    }

    OUTCOME_TRY(current_weight,
                weight_calculator_->calculateWeight(*heaviest_tipset_));

    if (weight > current_weight) {
      return takeHeaviestTipset(tipset);
    }

    return outcome::success();
  }

  outcome::result<void> ChainStoreImpl::takeHeaviestTipset(
      const Tipset &tipset) {
    OUTCOME_TRY(cids_json, encodeCidVector(tipset.key.cids()));

    if (!heaviest_tipset_.has_value()) {
      logger_->warn(
          "No heaviest tipset found, using provided tipset: {}, height: {}",
          cids_json,
          tipset.height());
      head_change_signal_(
          HeadChange{.type = HeadChangeType::CURRENT, .value = tipset});
      return writeHead(tipset);
    }

    if (tipset == *heaviest_tipset_) {
      logger_->warn(
          "provided tipset is equal to the current one, nothing happens");
      return outcome::success();
    }

    logger_->info(
        "New heaviest tipset {} (height={})", cids_json, tipset.height());

    OUTCOME_TRY(notifyHeadChange(*heaviest_tipset_, tipset));
    OUTCOME_TRY(writeHead(tipset));

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

  std::shared_ptr<ChainRandomnessProvider>
  ChainStoreImpl::createRandomnessProvider() {
    return std::make_shared<ChainRandomnessProviderImpl>(shared_from_this());
  }

  outcome::result<ChainPath> ChainStoreImpl::findChainPath(
      const Tipset &current, const Tipset &target) {
    // need to have genesis defined
    ChainPath path{};
    auto l = current;
    auto r = target;
    while (l != r) {
      if (l.height() > r.height()) {
        path.revert_chain.emplace_back(l);
        OUTCOME_TRY(key, l.getParents());
        OUTCOME_TRY(ts, loadTipset(key));
        l = std::move(ts);
      } else {
        path.apply_chain.emplace_front(r);
        OUTCOME_TRY(key, r.getParents());
        OUTCOME_TRY(ts, loadTipset(key));
        r = std::move(ts);
      }
    }
    return path;
  }

}  // namespace fc::storage::blockchain

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::blockchain, ChainStoreError, e) {
  using fc::storage::blockchain::ChainStoreError;

  switch (e) {
    case ChainStoreError::NO_MIN_TICKET_BLOCK:
      return "min ticket block has no value";
    case ChainStoreError::NO_HEAVIEST_TIPSET:
      return "no heaviest tipset in storage";
    case ChainStoreError::NO_GENESIS_BLOCK:
      return "no genesis block in storage";
    case ChainStoreError::STORE_NOT_INITIALIZED:
      return "store is not initialized properly";
  }

  return "ChainStoreError: unknown error";
}
