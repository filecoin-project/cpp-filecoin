/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/chain/impl/chain_store_impl.hpp"

#include "filecoin/common/outcome.hpp"
#include "filecoin/crypto/randomness/impl/chain_randomness_provider_impl.hpp"
#include "filecoin/primitives/address/address_codec.hpp"
#include "filecoin/primitives/cid/cid_of_cbor.hpp"
#include "filecoin/primitives/cid/json_codec.hpp"
#include "filecoin/primitives/tipset/tipset_key.hpp"
#include "filecoin/storage/chain/datastore_key.hpp"

namespace fc::storage::blockchain {
  using crypto::randomness::ChainRandomnessProviderImpl;
  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;

  namespace {
    const DatastoreKey chain_head_key{DatastoreKey::makeFromString("head")};
    const DatastoreKey genesis_key{DatastoreKey::makeFromString("0")};
  }  // namespace

  ChainStoreImpl::ChainStoreImpl(
      std::shared_ptr<ipfs::IpfsBlockService> block_service,
      std::shared_ptr<ChainDataStore> data_store,
      std::shared_ptr<BlockValidator> block_validator,
      std::shared_ptr<WeightCalculator> weight_calculator)
      : block_service_{std::move(block_service)},
        data_store_{std::move(data_store)},
        block_validator_{std::move(block_validator)},
        weight_calculator_{std::move(weight_calculator)} {
    logger_ = common::createLogger("chain store");
  }

  outcome::result<std::shared_ptr<ChainStoreImpl>> ChainStoreImpl::create(
      std::shared_ptr<ipfs::IpfsBlockService> block_store,
      std::shared_ptr<ChainDataStore> data_store,
      std::shared_ptr<BlockValidator> block_validator,

      std::shared_ptr<WeightCalculator> weight_calculator) {
    ChainStoreImpl tmp(std::move(block_store),
                       std::move(data_store),
                       std::move(block_validator),
                       std::move(weight_calculator));

    // TODO (yuraz): FIL-151 initialize notifications

    return std::make_shared<ChainStoreImpl>(std::move(tmp));
  }

  outcome::result<ChainStoreImpl::Tipset> ChainStoreImpl::loadTipset(
      const primitives::tipset::TipsetKey &key) {
    std::vector<BlockHeader> blocks;
    blocks.reserve(key.cids.size());
    // TODO (yuraz): FIL-151 check cache

    for (auto &cid : key.cids) {
      OUTCOME_TRY(block, getBlock(cid));
      blocks.push_back(std::move(block));
    }

    // TODO(yuraz): FIL-155 add tipset to cache before returning

    return Tipset::create(std::move(blocks));
  }

  outcome::result<BlockHeader> ChainStoreImpl::getBlock(const CID &cid) const {
    OUTCOME_TRY(bytes, block_service_->get(cid));
    return codec::cbor::decode<BlockHeader>(bytes);
  }

  outcome::result<void> ChainStoreImpl::load() {
    auto &&buffer = data_store_->get(chain_head_key);
    if (!buffer) {
      logger_->warn("no previous chain state found");
      return outcome::success();
    }

    OUTCOME_TRY(cids, codec::json::decodeCidVector(buffer.value()));
    auto &&ts_key = primitives::tipset::TipsetKey{std::move(cids)};
    OUTCOME_TRY(tipset, loadTipset(ts_key));
    heaviest_tipset_ = std::move(tipset);

    return outcome::success();
  }

  outcome::result<void> ChainStoreImpl::writeHead(
      const primitives::tipset::Tipset &tipset) {
    OUTCOME_TRY(data, codec::json::encodeCidVector(tipset.cids));
    OUTCOME_TRY(data_store_->set(chain_head_key, data));

    return outcome::success();
  }

  outcome::result<void> ChainStoreImpl::addBlock(const BlockHeader &block) {
    OUTCOME_TRY(persistBlockHeaders({std::ref(block)}));
    OUTCOME_TRY(tipset, expandTipset(block));
    OUTCOME_TRY(updateHeavierTipset(tipset));

    return outcome::success();
  }

  outcome::result<void> ChainStoreImpl::persistBlockHeaders(
      const std::vector<std::reference_wrapper<const BlockHeader>>
          &block_headers) {
    for (auto &b : block_headers) {
      OUTCOME_TRY(data, codec::cbor::encode(b));
      OUTCOME_TRY(cid, common::getCidOf(data));
      OUTCOME_TRY(
          block_service_->set(std::move(cid), common::Buffer{std::move(data)}));
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

    std::map<primitives::address::Address, bool> inclMiners;
    inclMiners[block_header.miner] = true;
    OUTCOME_TRY(block_cid, primitives::cid::getCidOfCbor(block_header));
    for (auto &&c : tipsets) {
      if (c == block_cid) {
        continue;
      }

      OUTCOME_TRY(h, getBlock(c));

      if (inclMiners.find(h.miner) != std::end(inclMiners)) {
        auto &&miner_address = primitives::address::encodeToString(h.miner);
        logger_->warn(
            "Have multiple blocks from miner {} at height {} in our tipset "
            "cache",
            miner_address,
            h.height);
        continue;
      }

      if (h.parents == block_header.parents) {
        all_headers.push_back(h);
      }
    }

    return Tipset::create(all_headers);
  }

  outcome::result<void> ChainStoreImpl::updateHeavierTipset(
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
  }

  return "ChainStoreError: unknown error";
}
