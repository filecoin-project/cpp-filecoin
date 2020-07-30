/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset.hpp"

#include "common/logger.hpp"
#include "primitives/cid/cid_of_cbor.hpp"

#include "crypto/blake2/blake2b.h"
#include "crypto/blake2/blake2b160.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::tipset, TipsetError, e) {
  using fc::primitives::tipset::TipsetError;
  switch (e) {
    case (TipsetError::kNoBlocks):
      return "Need to have at least one block to create tipset";
    case TipsetError::kMismatchingHeights:
      return "Cannot create tipset, mismatching blocks heights";
    case TipsetError::kMismatchingParents:
      return "Cannot create tipset, mismatching block parents";
    case TipsetError::kTicketHasNoValue:
      return "An optional ticket is not initialized";
    case TipsetError::kTicketsCollision:
      return "Duplicate tickets in tipset";
    case TipsetError::kBlockOrderFailure:
      return "Wrong order of blocks in tipset";
    case TipsetError::kNoBeacons:
      return "No beacons in chain";
  }
  return "Unknown tipset error";
}

namespace fc::primitives::tipset {

  using TicketHash =
      std::array<uint8_t, crypto::blake2b::BLAKE2B256_HASH_LENGTH>;

  namespace {

    outcome::result<TicketHash> ticketHash(const block::BlockHeader &hdr) {
      if (hdr.height == 0) {
        // Genesis block may not have ticket
        return TicketHash{};
      }

      if (!hdr.ticket.has_value()) {
        return TipsetError::kTicketHasNoValue;
      }

      blake2b_ctx ctx;

      if (blake2b_init(
              &ctx, crypto::blake2b::BLAKE2B256_HASH_LENGTH, nullptr, 0)
          != 0) {
        return HashError::HASH_INITIALIZE_ERROR;
      }

      const auto &ticket_bytes = hdr.ticket.value().bytes;

      blake2b_update(&ctx, ticket_bytes.data(), ticket_bytes.size());
      TicketHash hash;
      blake2b_final(&ctx, hash.data());
      return hash;
    }

    int compareHashes(const TicketHash &a, const TicketHash &b) {
      return memcmp(
          a.data(), b.data(), crypto::blake2b::BLAKE2B256_HASH_LENGTH);
    }

  }  // namespace

  outcome::result<void> MessageVisitor::visit(const BlockHeader &block,
                                              const Visitor &visitor) {
    auto onMessage = [&](auto bls, auto &cid) -> outcome::result<void> {
      if (visited.insert(cid).second) {
        OUTCOME_TRY(visitor(visited.size() - 1, bls, cid));
      }
      return outcome::success();
    };
    OUTCOME_TRY(meta, ipld->getCbor<block::MsgMeta>(block.messages));
    OUTCOME_TRY(meta.bls_messages.visit(
        [&](auto, auto &cid) { return onMessage(true, cid); }));
    OUTCOME_TRY(meta.secp_messages.visit(
        [&](auto, auto &cid) { return onMessage(false, cid); }));
    return outcome::success();
  }

  outcome::result<void> TipsetCreator::canExpandTipset(
      const block::BlockHeader &hdr) const {
    if (blks_.empty()) {
      return outcome::success();
    }

    if (hdr.height > 0 && !hdr.ticket.has_value()) {
      return TipsetError::kTicketHasNoValue;
    }

    const auto &first_block = blks_[0];

    if (hdr.height != first_block.height) {
      return TipsetError::kMismatchingHeights;
    }

    if (hdr.parents != first_block.parents) {
      return TipsetError::kMismatchingParents;
    }

    return outcome::success();
  }

  outcome::result<void> TipsetCreator::expandTipset(block::BlockHeader hdr) {
    OUTCOME_TRY(cid, fc::primitives::cid::getCidOfCbor(hdr));
    return expandTipset(std::move(cid), std::move(hdr));
  }

  outcome::result<void> TipsetCreator::expandTipset(CID cid,
                                                    block::BlockHeader hdr) {
    // must be called prior to expand()
    assert(canExpandTipset(hdr));

    constexpr auto kReserveSize = 5;

    if (blks_.empty()) {
      blks_.reserve(kReserveSize);
      cids_.reserve(kReserveSize);
      ticket_hashes_.reserve(kReserveSize);
      OUTCOME_TRY(hash, ticketHash(hdr));
      ticket_hashes_.emplace_back(std::move(hash));
      cids_.emplace_back(std::move(cid));
      blks_.emplace_back(std::move(hdr));
      return outcome::success();
    }

    auto it = ticket_hashes_.begin();
    OUTCOME_TRY(ticket_hash, ticketHash(hdr));
    size_t idx = 0;
    for (auto e = ticket_hashes_.end(); it != e; ++it, ++idx) {
      int c = compareHashes(ticket_hash, *it);
      if (c == 0) {
        return TipsetError::kTicketsCollision;
      }
      if (c < 0) {
        continue;
      }
      break;
    }

    if (++it == ticket_hashes_.end()) {
      // most likely they come in proper order
      blks_.push_back(std::move(hdr));
      cids_.push_back(std::move(cid));
      ticket_hashes_.push_back(std::move(ticket_hash));
    } else {
      blks_.insert(blks_.begin() + idx, std::move(hdr));
      cids_.insert(cids_.begin() + idx, std::move(cid));
      ticket_hashes_.insert(ticket_hashes_.begin() + idx,
                            std::move(ticket_hash));
    }

    return outcome::success();
  }

  TipsetCPtr TipsetCreator::getTipset(bool clear) {
    if (blks_.empty()) {
      return std::make_shared<Tipset>();
    }
    if (clear) {
      OUTCOME_EXCEPT(key, TipsetKey::create(std::move(cids_)));
      return std::make_shared<Tipset>(std::move(key), std::move(blks_));
    }

    // make copy, don't erase
    OUTCOME_EXCEPT(key, TipsetKey::create(cids_));
    return std::make_shared<Tipset>(key, blks_);
  }

  void TipsetCreator::clear() {
    blks_.clear();
    cids_.clear();
    ticket_hashes_.clear();
  }

  uint64_t TipsetCreator::height() const {
    return blks_.empty() ? 0 : blks_[0].height;
  }

  outcome::result<TipsetCPtr> Tipset::create(const TipsetHash &hash,
                                             BlocksFromNetwork blocks) {
    TipsetCreator creator;

    for (auto &b : blocks) {
      if (!b.has_value()) {
        return TipsetError::kNoBlocks;
      }

      auto &hdr = b.value();
      OUTCOME_TRY(creator.canExpandTipset(hdr));
      OUTCOME_TRY(creator.expandTipset(std::move(hdr)));
    }

    TipsetCPtr tipset = creator.getTipset(true);
    if (tipset->key.hash() != hash) {
      return TipsetError::kBlockOrderFailure;
    }

    return std::move(tipset);
  }

  outcome::result<TipsetCPtr> Tipset::create(
      std::vector<block::BlockHeader> blocks) {
    TipsetCreator creator;

    for (auto &hdr : blocks) {
      OUTCOME_TRY(creator.canExpandTipset(hdr));
      OUTCOME_TRY(creator.expandTipset(std::move(hdr)));
    }

    return creator.getTipset(true);
  }

  outcome::result<TipsetCPtr> Tipset::load(Ipld &ipld,
                                           const std::vector<CID> &cids) {
    std::vector<BlockHeader> blocks;
    blocks.reserve(cids.size());
    for (auto &cid : cids) {
      OUTCOME_TRY(block, ipld.getCbor<BlockHeader>(cid));
      blocks.emplace_back(std::move(block));
    }
    return create(std::move(blocks));
  }

  outcome::result<TipsetCPtr> Tipset::loadGenesis(Ipld &ipld, const CID &cid) {
    BlockHeader block;
    OUTCOME_TRY(bytes, ipld.get(cid));
    std::vector<std::vector<uint8_t>> dummy;
    try {
      codec::cbor::CborDecodeStream decoder(bytes);
      decoder.list() >> block.miner >> dummy >> block.election_proof
          >> block.beacon_entries >> block.win_post_proof >> block.parents
          >> block.parent_weight >> block.height >> block.parent_state_root
          >> block.parent_message_receipts >> block.messages
          >> block.bls_aggregate >> block.timestamp >> block.block_sig
          >> block.fork_signaling;

    } catch (std::system_error &e) {
      return e.code();
    }

    OUTCOME_TRY(key, TipsetKey::create({std::move(cid)}));
    std::vector<BlockHeader> blocks{std::move(block)};
    return std::make_shared<Tipset>(std::move(key), std::move(blocks));
  }

  outcome::result<TipsetCPtr> Tipset::loadParent(Ipld &ipld) const {
    return load(ipld, blks[0].parents);
  }

  outcome::result<BeaconEntry> Tipset::latestBeacon(Ipld &ipld) const {
    auto ts{this};
    TipsetCPtr parent;
    // TODO: magic number from lotus
    for (auto i{0}; i < 20; ++i) {
      auto beacons{ts->blks[0].beacon_entries};
      if (!beacons.empty()) {
        return *beacons.rbegin();
      }
      if (ts->height() == 0) {
        break;
      }
      OUTCOME_TRYA(parent, ts->loadParent(ipld));
      ts = parent.get();
    }
    return TipsetError::kNoBeacons;
  }

  outcome::result<void> Tipset::visitMessages(
      IpldPtr ipld, const MessageVisitor::Visitor &visitor) const {
    MessageVisitor message_visitor{ipld};
    for (auto &block : blks) {
      OUTCOME_TRY(message_visitor.visit(block, visitor));
    }
    return outcome::success();
  }

  outcome::result<Randomness> Tipset::randomness(
      Ipld &ipld,
      DomainSeparationTag tag,
      ChainEpoch round,
      gsl::span<const uint8_t> entropy) const {
    auto ts{this};
    TipsetCPtr parent;
    while (ts->height() != 0 && static_cast<ChainEpoch>(ts->height()) > round) {
      OUTCOME_TRYA(parent, ts->loadParent(ipld));
      ts = parent.get();
    }
    return crypto::randomness::drawRandomness(
        ts->getMinTicketBlock().ticket->bytes, tag, round, entropy);
  }

  outcome::result<TipsetKey> Tipset::getParents() const {
    return blks.empty() ? TipsetKey() : TipsetKey::create(blks[0].parents);
  }

  uint64_t Tipset::getMinTimestamp() const {
    if (blks.empty()) return 0;
    return std::min_element(blks.begin(),
                            blks.end(),
                            [](const auto &b1, const auto &b2) -> bool {
                              return b1.timestamp < b2.timestamp;
                            })
        ->timestamp;
  }

  const block::BlockHeader &Tipset::getMinTicketBlock() const {
    // i believe that Tipset::create sorts them
    return blks[0];
  }

  const CID &Tipset::getParentStateRoot() const {
    return blks[0].parent_state_root;
  }

  const BigInt &Tipset::getParentWeight() const {
    static const BigInt zero;
    return blks.empty() ? zero : blks[0].parent_weight;
  }

  const CID &Tipset::getParentMessageReceipts() const {
    return blks[0].parent_message_receipts;
  }

  uint64_t Tipset::height() const {
    return blks.empty() ? 0 : blks[0].height;
  }

  bool Tipset::contains(const CID &cid) const {
    const auto &cids = key.cids();
    return std::find(cids.begin(), cids.end(), cid) != std::end(cids);
  }

  bool operator==(const Tipset &lhs, const Tipset &rhs) {
    if (lhs.blks.size() != rhs.blks.size()) return false;
    return std::equal(lhs.blks.begin(), lhs.blks.end(), rhs.blks.begin());
  }

  bool operator!=(const Tipset &l, const Tipset &r) {
    return !(l == r);
  }
}  // namespace fc::primitives::tipset

namespace fc::codec::cbor {

  namespace {
    struct TipsetDecodeCandidate {
      std::vector<CID> cids;
      std::vector<fc::primitives::block::BlockHeader> blks;
      uint64_t height;
    };

    CBOR_TUPLE(TipsetDecodeCandidate, cids, blks, height);

  }  // namespace

  template <>
  outcome::result<fc::primitives::tipset::TipsetCPtr>
  decode<fc::primitives::tipset::TipsetCPtr>(gsl::span<const uint8_t> input) {
    using namespace fc::primitives::tipset;

    OUTCOME_TRY(decoded, decode<TipsetDecodeCandidate>(input));
    if (decoded.blks.empty() && decoded.height != 0) {
      return TipsetError::kMismatchingHeights;
    }
    OUTCOME_TRY(tipset, Tipset::create(std::move(decoded.blks)));
    if (tipset->key.cids() != decoded.cids) {
      return TipsetError::kBlockOrderFailure;
    }
    return std::move(tipset);
  }

}  // namespace fc::codec::cbor
