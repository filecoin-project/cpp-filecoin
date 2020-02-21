/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/chain_store.hpp"

#include "common/le_encoder.hpp"
#include "common/outcome.hpp"
#include "datastore_key.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "primitives/cid/json_codec.hpp"
#include "primitives/persistent_block/persistent_block.hpp"
#include "primitives/tipset/tipset_key.hpp"

namespace fc::storage::blockchain {
  using primitives::block::BlockHeader;
  using primitives::blockchain::block::PersistentBlock;
  using primitives::tipset::Tipset;

  namespace {
    const DatastoreKey chain_head_key{DatastoreKey::makeFromString("head")};
    const DatastoreKey genesis_key{DatastoreKey::makeFromString("0")};
  }  // namespace

  ChainStore::ChainStore(std::shared_ptr<ipfs::IpfsBlockService> block_service,
                         std::shared_ptr<ChainDataStore> data_store,
                         std::shared_ptr<BlockValidator> block_validator,
                         std::shared_ptr<WeightCalculator> weight_calculator)
      : block_service_{std::move(block_service)},
        data_store_{std::move(data_store)},
        block_validator_{std::move(block_validator)},
        weight_calculator_{std::move(weight_calculator)} {
    logger_ = common::createLogger("chain store");
  }

  outcome::result<std::shared_ptr<ChainStore>> ChainStore::create(
      std::shared_ptr<ipfs::IpfsBlockService> block_store,
      std::shared_ptr<ChainDataStore> data_store,
      std::shared_ptr<BlockValidator> block_validator,
      std::shared_ptr<WeightCalculator> weight_calculator) {
    std::shared_ptr<ChainStore> cs{};
    ChainStore tmp(std::move(block_store),
                   std::move(data_store),
                   std::move(block_validator),
                   std::move(weight_calculator));
    cs.reset(new ChainStore(std::move(tmp)));

    // TODO (yuraz): FIL-151 initialize notifications

    return cs;
  }

  outcome::result<ChainStore::Tipset> ChainStore::loadTipset(
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

  outcome::result<BlockHeader> ChainStore::getBlock(const CID &cid) const {
    OUTCOME_TRY(bytes, block_service_->get(cid));
    return codec::cbor::decode<BlockHeader>(bytes);
  }

  outcome::result<void> ChainStore::load() {
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

  outcome::result<void> ChainStore::writeHead(
      const primitives::tipset::Tipset &tipset) {
    OUTCOME_TRY(data, codec::json::encodeCidVector(tipset.cids));
    OUTCOME_TRY(data_store_->set(chain_head_key, data));

    return outcome::success();
  }

  outcome::result<void> ChainStore::addBlock(const BlockHeader &block) {
    OUTCOME_TRY(persistBlockHeaders({std::ref(block)}));
    OUTCOME_TRY(tipset, expandTipset(block));
    OUTCOME_TRY(updateHeavierTipset(tipset));

    return outcome::success();
  }

  outcome::result<void> ChainStore::persistBlockHeaders(
      const std::vector<std::reference_wrapper<const BlockHeader>>
          &block_headers) {
    for (auto &b : block_headers) {
      OUTCOME_TRY(data, codec::cbor::encode(b));
      OUTCOME_TRY(cid, common::getCidOf(data));
      OUTCOME_TRY(block_service_->set(std::move(cid), common::Buffer{std::move(data)}));
    }

    return outcome::success();
  }

  outcome::result<Tipset> ChainStore::expandTipset(
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

  outcome::result<void> ChainStore::updateHeavierTipset(const Tipset &tipset) {
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

  outcome::result<void> ChainStore::takeHeaviestTipset(const Tipset &tipset) {
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

  namespace {
    /**
     * @brief ChainStore needs its own randomnes calculation function
     * @return randomness value
     */
    crypto::randomness::Randomness drawRandomness(
        const primitives::ticket::Ticket &ticket, uint64_t round) {
      common::Buffer buffer{};
      const size_t bytes_required = sizeof(round) + ticket.bytes.size();
      buffer.reserve(bytes_required);

      buffer.put(ticket.bytes);
      common::encodeLebInteger(round, buffer);

      auto hash = libp2p::crypto::sha256(buffer);

      return crypto::randomness::Randomness(hash);
    }
  }  // namespace

  outcome::result<ChainStore::Randomness> ChainStore::sampleRandomness(
      const std::vector<CID> &blks, uint64_t round) {
    std::reference_wrapper<const std::vector<CID>> cids{blks};

    while (true) {
      OUTCOME_TRY(tipset, loadTipset(TipsetKey{cids.get()}));
      OUTCOME_TRY(min_ticket_block, tipset.getMinTicketBlock());

      if (tipset.height <= round) {
        if (!min_ticket_block.get().ticket.has_value()) {
          return ChainStoreError::NO_MIN_TICKET_BLOCK;
        }

        return drawRandomness(*min_ticket_block.get().ticket, round);
      }

      // special case for lookback behind genesis block
      if (min_ticket_block.get().height == 0) {
        // round is negative
        auto &&negative_hash =
            drawRandomness(*min_ticket_block.get().ticket, round - 1);
        // for negative lookbacks, just use the hash of the positive ticket hash
        // value
        auto &&positive_hash = libp2p::crypto::sha256(negative_hash);
        return Randomness{positive_hash};
      }

      // TODO (yuraz) I know it's very ugly, and needs to be refactored
      // I translated it directly from go to C++
      cids = min_ticket_block.get().parents;
    }
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
