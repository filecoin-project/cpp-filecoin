/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset.hpp"

#include "common/logger.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/cid_of_cbor.hpp"

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
    case TipsetError::kNoBeacons:
      return "No beacons in chain";
  }
  return "Unknown tipset error";
}

namespace fc::primitives::tipset {
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

  outcome::result<Tipset> Tipset::create(std::vector<BlockHeader> blocks) {
    // required to have at least one block
    if (blocks.empty()) {
      return TipsetError::kNoBlocks;
    }

    // check for blocks consistency
    const auto height0 = blocks[0].height;
    const auto &parents = blocks[0].parents;
    for (size_t i = 1; i < blocks.size(); ++i) {
      const auto &b = blocks[i];
      if (height0 != b.height) {
        return TipsetError::kMismatchingHeights;
      }
      if (parents != b.parents) {
        return TipsetError::kMismatchingParents;
      }
    }

    std::vector<std::pair<block::BlockHeader, CID>> items;
    items.reserve(blocks.size());
    for (auto &block : blocks) {
      assert(block.ticket);
      OUTCOME_TRY(cid, fc::primitives::cid::getCidOfCbor(block));
      OUTCOME_TRY(cid.toBytes());
      // need to ensure that all cids are calculated before sort,
      // since it will terminate program in case of exception
      items.emplace_back(std::make_pair(std::move(block), cid));
    }

    // the sort function shouldn't throw exceptions
    // if an exception is thrown from std::sort, program will be terminated
    std::sort(
        items.begin(),
        items.end(),
        [logger = common::createLogger("tipset")](auto &p1, auto &p2) -> bool {
          if (&p1 == &p2) {
            return false;
          }
          auto &[b1, cid1] = p1;
          auto &[b2, cid2] = p2;
          auto &t1 = b1.ticket;
          auto &t2 = b2.ticket;
          if (t1 == t2) {
            logger->warn("blocks have same ticket ({} {})",
                         b1.height,
                         b1.miner,
                         b2.miner);
            return cid1.toBytes().value() < cid2.toBytes().value();
          }
          return crypto::blake2b::blake2b_256(t1->bytes)
                 < crypto::blake2b::blake2b_256(t2->bytes);
        });

    Tipset ts{};
    ts.height = height0;
    ts.cids.reserve(items.size());
    ts.blks.reserve(items.size());

    for (auto &[b, c] : items) {
      ts.blks.push_back(std::move(b));
      ts.cids.push_back(std::move(c));
    }

    return ts;
  }

  outcome::result<Tipset> Tipset::load(Ipld &ipld,
                                       const std::vector<CID> &cids) {
    std::vector<BlockHeader> blocks;
    blocks.reserve(cids.size());
    for (auto &cid : cids) {
      OUTCOME_TRY(block, ipld.getCbor<BlockHeader>(cid));
      blocks.emplace_back(std::move(block));
    }
    return create(std::move(blocks));
  }

  outcome::result<Tipset> Tipset::loadParent(Ipld &ipld) const {
    return load(ipld, blks[0].parents);
  }

  outcome::result<BeaconEntry> Tipset::latestBeacon(Ipld &ipld) const {
    auto ts{this};
    Tipset parent;
    // TODO: magic number from lotus
    for (auto i{0}; i < 20; ++i) {
      auto beacons{ts->blks[0].beacon_entries};
      if (!beacons.empty()) {
        return *beacons.rbegin();
      }
      if (ts->height == 0) {
        break;
      }
      OUTCOME_TRYA(parent, ts->loadParent(ipld));
      ts = &parent;
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

  outcome::result<Randomness> Tipset::beaconRandomness(
      Ipld &ipld,
      DomainSeparationTag tag,
      ChainEpoch round,
      gsl::span<const uint8_t> entropy) const {
    auto ts{this};
    Tipset parent;
    while (ts->height != 0 && static_cast<ChainEpoch>(ts->height) > round) {
      OUTCOME_TRYA(parent, ts->loadParent(ipld));
      ts = &parent;
    }
    OUTCOME_TRY(beacon, ts->latestBeacon(ipld));
    return crypto::randomness::drawRandomness(beacon.data, tag, round, entropy);
  }

  outcome::result<Randomness> Tipset::ticketRandomness(
      Ipld &ipld,
      DomainSeparationTag tag,
      ChainEpoch round,
      gsl::span<const uint8_t> entropy) const {
    auto ts{this};
    Tipset parent;
    while (ts->height != 0 && static_cast<ChainEpoch>(ts->height) > round) {
      OUTCOME_TRYA(parent, ts->loadParent(ipld));
      ts = &parent;
    }
    return crypto::randomness::drawRandomness(
        ts->getMinTicketBlock().ticket->bytes, tag, round, entropy);
  }

  TipsetKey Tipset::getParents() const {
    return blks[0].parents;
  }

  outcome::result<TipsetKey> Tipset::makeKey() const {
    return TipsetKey{cids};
  }

  uint64_t Tipset::getMinTimestamp() const {
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

  const CID &Tipset::getParentMessageReceipts() const {
    return blks[0].parent_message_receipts;
  }

  const BigInt &Tipset::getParentWeight() const {
    return blks[0].parent_weight;
  }

  const BigInt &Tipset::getParentBaseFee() const {
    return blks[0].parent_base_fee;
  }

  bool Tipset::contains(const CID &cid) const {
    return std::find(cids.begin(), cids.end(), cid) != std::end(cids);
  }

  bool operator==(const Tipset &lhs, const Tipset &rhs) {
    return lhs.blks == rhs.blks;
  }

  bool operator!=(const Tipset &l, const Tipset &r) {
    return !(l == r);
  }
}  // namespace fc::primitives::tipset
