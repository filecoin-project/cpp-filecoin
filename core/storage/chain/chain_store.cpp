/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/chain_store.hpp"

#include "common/le_encoder.hpp"
#include "common/outcome.hpp"
#include "datastore_key.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "primitives/cid/json_codec.hpp"
#include "primitives/tipset/tipset_key.hpp"

namespace fc::storage::chain {
  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;

  namespace {
    const DatastoreKey chain_head_key = makeKeyFromString("head");
  }

  ChainStore::ChainStore(std::shared_ptr<ipfs::BlockService> block_service,
                         std::shared_ptr<ChainDataStore> data_store,
                         std::shared_ptr<BlockValidator> block_validator)
      : block_service_{std::move(block_service)},
        data_store_{std::move(data_store)},
        block_validator_{std::move(block_validator)} {
    logger_ = common::createLogger("chain store");
  }

  outcome::result<std::shared_ptr<ChainStore>> ChainStore::create(
      std::shared_ptr<ipfs::BlockService> block_store,
      std::shared_ptr<ChainDataStore> data_store,
      std::shared_ptr<BlockValidator> block_validator) {
    std::shared_ptr<ChainStore> cs{};
    ChainStore tmp(std::move(block_store),
                   std::move(data_store),
                   std::move(block_validator));
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
    OUTCOME_TRY(bytes, block_service_->getBlockContent(cid));
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
    return data_store_->set(chain_head_key, data);
  }

  /**
   * @brief chain store needs its own randomnes calculation function
   * @return randomness value
   */
  crypto::randomness::Randomness drawRandomnessInternal(
      const primitives::ticket::Ticket &ticket, uint64_t round) {
    common::Buffer buffer{};
    const size_t bytes_required = sizeof(round) + sizeof(ticket.bytes);
    buffer.reserve(bytes_required);

    buffer.put(ticket.bytes);
    common::encodeLebInteger(round, buffer);

    auto hash = libp2p::crypto::sha256(buffer);

    return crypto::randomness::Randomness(hash);
  }

  // TODO (yuraz): implement
  outcome::result<ChainStore::Randomness> ChainStore::drawRandomness(
      const std::vector<CID> &blks, uint64_t round) {
    std::reference_wrapper<const std::vector<CID>> cids{blks};

    while (true) {
      OUTCOME_TRY(tipset, loadTipset(TipsetKey{cids.get()}));
      OUTCOME_TRY(min_ticket_block, tipset.getMinTicketBlock());

      if (tipset.height <= round) {
        if (!min_ticket_block.get().ticket.has_value()) {
          return ChainStoreError::NO_MIN_TICKET_BLOCK;
        }

        return drawRandomnessInternal(*min_ticket_block.get().ticket, round);
      }

      // special case for lookback behind genesis block
      if (min_ticket_block.get().height == 0) {
        // round is negative
        auto &&negative_hash =
            drawRandomnessInternal(*min_ticket_block.get().ticket, round - 1);
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
}  // namespace fc::storage::chain

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::chain, ChainStoreError, e) {
  using fc::storage::chain::ChainStoreError;

  switch (e) {
    case ChainStoreError::NO_MIN_TICKET_BLOCK:
      return "min ticket block has no value";
  }

  return "ChainStoreError: unknown error";
}
