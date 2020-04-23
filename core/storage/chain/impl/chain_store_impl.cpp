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
  using crypto::randomness::ChainRandomnessProviderImpl;
  using primitives::address::Address;
  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;

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

    // TODO (yuraz): FIL-151 initialize notifications

    return std::make_shared<ChainStoreImpl>(std::move(tmp));
  }

  outcome::result<Tipset> ChainStoreImpl::loadTipset(
      const primitives::tipset::TipsetKey &key) const {
    auto it = tipsets_cache_.find(key);
    if (it != tipsets_cache_.end()) {
      return it->second;
    }

    std::vector<BlockHeader> blocks;
    blocks.reserve(key.cids.size());
    // TODO (yuraz): FIL-151 check cache

    for (auto &cid : key.cids) {
      OUTCOME_TRY(block, getCbor<BlockHeader>(cid));
      blocks.push_back(std::move(block));
    }

    // TODO(yuraz): FIL-155 add tipset to cache before returning
    OUTCOME_TRY(tipset, Tipset::create(std::move(blocks)));
    tipsets_cache_[key] = tipset;
    return tipset;
  }

  outcome::result<void> ChainStoreImpl::load() {
    auto &&buffer = chain_data_store_->get(kChainHeadKey);
    if (!buffer) {
      logger_->warn("no previous chain state found");
      return outcome::success();
    }

    OUTCOME_TRY(cids, codec::json::decodeCidVector(buffer.value()));
    OUTCOME_TRY(ts_key, primitives::tipset::TipsetKey::create(std::move(cids)));
    OUTCOME_TRY(tipset, loadTipset(ts_key));
    heaviest_tipset_ = std::move(tipset);

    return outcome::success();
  }

  outcome::result<Tipset> ChainStoreImpl::heaviestTipset() const {
    if (heaviest_tipset_) {
      return *heaviest_tipset_;
    }
    return ChainStoreError::NO_HEAVIEST_TIPSET;
  }

  outcome::result<bool> ChainStoreImpl::containsTipset(
      const TipsetKey &key) const {
    if (tipsets_cache_.count(key) > 0) {
      return true;
    }

    // TODO: (yuraz) implement something better
    OUTCOME_TRY(loadTipset(key));
    return true;
  }

  outcome::result<void> ChainStoreImpl::storeTipset(const Tipset &tipset) {
    std::vector<std::reference_wrapper<const BlockHeader>> refs;
    refs.reserve(tipset.blks.size());
    for (const auto &b : tipset.blks) {
      refs.emplace_back(std::ref(b));
    }

    return persistBlockHeaders(refs);
  }

  outcome::result<BlockHeader> ChainStoreImpl::getGenesis() const {
    OUTCOME_TRY(buffer, chain_data_store_->get(kGenesisKey));

    OUTCOME_TRY(cids, codec::json::decodeCidVector(buffer));
    BOOST_ASSERT_MSG(cids.size() == 1, "wrong genesis key format");
    return getCbor<BlockHeader>(cids[0]);
  }

  outcome::result<void> ChainStoreImpl::setGenesis(
      const BlockHeader &block_header) {
    OUTCOME_TRY(genesis_tipset, Tipset::create({block_header}));
    OUTCOME_TRY(storeTipset(genesis_tipset));
    OUTCOME_TRY(
        buffer,
        codec::json::encodeCidVector(std::vector<CID>{genesis_tipset.cids[0]}));

    return chain_data_store_->set(kGenesisKey, buffer);
  }

  outcome::result<void> ChainStoreImpl::writeHead(
      const primitives::tipset::Tipset &tipset) {
    OUTCOME_TRY(data, codec::json::encodeCidVector(tipset.cids));
    return chain_data_store_->set(kChainHeadKey, data);
  }

  outcome::result<void> ChainStoreImpl::addBlock(const BlockHeader &block) {
    OUTCOME_TRY(persistBlockHeaders({std::ref(block)}));
    OUTCOME_TRY(tipset, expandTipset(block));
    return updateHeaviestTipset(tipset);
  }

  outcome::result<void> ChainStoreImpl::persistBlockHeaders(
      const std::vector<std::reference_wrapper<const BlockHeader>>
          &block_headers) {
    for (auto &b : block_headers) {
      OUTCOME_TRY(data, codec::cbor::encode(b));
      OUTCOME_TRY(cid, common::getCidOf(data));
      OUTCOME_TRY(data_store_->set(cid, std::move(data)));
    }

    return outcome::success();
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
    OUTCOME_TRY(block_cid, primitives::cid::getCidOfCbor(block_header));
    for (auto &&cid : tipsets) {
      if (cid == block_cid) {
        continue;
      }

      OUTCOME_TRY(bh, getCbor<BlockHeader>(cid));

      if (included_miners.count(bh.miner) > 0) {
        auto &&miner_address = primitives::address::encodeToString(bh.miner);
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
    if (heaviest_tipset_.has_value()) {
      // TODO(yuraz): FIL-151 notify head change
      // use lotus implementation for reference
      // https://github.com/filecoin-project/lotus/blob/955b755055ecf64ce7a9ff8749db4280c737406c/chain/store/store.go#L296
    } else {
      logger_->warn("No heaviest tipset found, using provided tipset");
    }

    OUTCOME_TRY(cids_json, codec::json::encodeCidVector(tipset.cids));
    logger_->info(
        "New heaviest tipset {} (height={})", cids_json, tipset.height);
    heaviest_tipset_ = tipset;
    OUTCOME_TRY(writeHead(tipset));

    return outcome::success();
  }

  std::shared_ptr<fc::crypto::randomness::ChainRandomnessProvider>
  ChainStoreImpl::createRandomnessProvider() {
    return std::make_shared<
        ::fc::crypto::randomness::ChainRandomnessProviderImpl>(
        shared_from_this());
  }

}  // namespace fc::storage::blockchain

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::blockchain, ChainStoreError, e) {
  using fc::storage::blockchain::ChainStoreError;

  switch (e) {
    case ChainStoreError::NO_MIN_TICKET_BLOCK:
      return "min ticket block has no value";
    case ChainStoreError::NO_HEAVIEST_TIPSET:
      return "no heaviest tipset";
    case ChainStoreError::STORE_NOT_INITIALIZED:
      return "store is not initialized properly";
  }

  return "ChainStoreError: unknown error";
}
