/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset.hpp"

#include "common/logger.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/cid_of_cbor.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::tipset, TipsetError, e) {
  using fc::primitives::tipset::TipsetError;
  switch (e) {
    case (TipsetError::NO_BLOCKS):
      return "No blocks to create tipset";
    case TipsetError::MISMATCHING_HEIGHTS:
      return "Cannot create tipset, mismatching blocks heights";
    case TipsetError::MISMATCHING_PARENTS:
      return "Cannot create tipset, mismatching block parents";
    case TipsetError::TICKET_HAS_NO_VALUE:
      return "An optional ticket is not initialized";
    case TipsetError::TICKETS_COLLISION:
      return "Duplicate tickets in tipset";
    case TipsetError::BLOCK_ORDER_FAILURE:
      return "Wrong order of blocks in tipset";
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

  outcome::result<Tipset> Tipset::create(TipsetKey key,
                                         BlocksAvailable blocks) {
    if (blocks.empty() || !blocks[0].has_value()
        || key.cids().size() != blocks.size()) {
      return TipsetError::NO_BLOCKS;
    }
    const auto &block0 = blocks[0].value();
    if (!block0.ticket.has_value()) {
      return TipsetError::TICKET_HAS_NO_VALUE;
    }
    auto height = block0.height;

    std::vector<block::BlockHeader> b;
    b.reserve(blocks.size());
    b.push_back(std::move(blocks[0].value()));

    for (size_t i = 1; i < blocks.size(); ++i) {
      const auto &block_opt = blocks[i];
      if (!block_opt.has_value()) {
        return TipsetError::NO_BLOCKS;
      }
      const auto &block = block_opt.value();
      if (!block.ticket.has_value()) {
        return TipsetError::TICKET_HAS_NO_VALUE;
      }
      if (block.height != height) {
        return TipsetError::MISMATCHING_HEIGHTS;
      }
      if (block.parents != b.back().parents) {
        return TipsetError::MISMATCHING_PARENTS;
      }
      if (block.ticket.value() < b.back().ticket.value()) {
        return TipsetError::BLOCK_ORDER_FAILURE;
      }
      if (block.ticket.value() == b.back().ticket.value()) {
        return TipsetError::TICKETS_COLLISION;
      }
      b.push_back(std::move(blocks[i].value()));
    }

    return Tipset{std::move(key), std::move(b)};
  }

  outcome::result<Tipset> Tipset::create(
      std::vector<block::BlockHeader> blocks) {
    // required to have at least one block
    if (blocks.empty()) {
      return TipsetError::NO_BLOCKS;
    }

    // check for blocks consistency
    const auto height0 = blocks[0].height;
    const auto &parents = blocks[0].parents;
    for (size_t i = 1; i < blocks.size(); ++i) {
      const auto &b = blocks[i];
      if (height0 != b.height) {
        return TipsetError::MISMATCHING_HEIGHTS;
      }
      if (parents != b.parents) {
        return TipsetError::MISMATCHING_PARENTS;
      }
    }

    std::vector<std::pair<block::BlockHeader, CID>> items;
    items.reserve(blocks.size());
    for (auto &block : blocks) {
      assert(block.ticket);
      OUTCOME_TRY(cid, fc::primitives::cid::getCidOfCbor(block));
      // need to ensure that all cids are calculated before sort,
      // since it will terminate program in case of exception
      items.emplace_back(std::make_pair(std::move(block), cid));
    }

    // the sort function shouldn't throw exceptions
    // if an exception is thrown from std::sort, program will be terminated
    std::sort(items.begin(),
              items.end(),
              [logger = common::createLogger("tipset")](
                  const auto &p1, const auto &p2) -> bool {
                auto &[b1, cid1] = p1;
                auto &[b2, cid2] = p2;
                const auto &t1 = b1.ticket;
                const auto &t2 = b2.ticket;
                if (b1.ticket == b2.ticket) {
                  logger->warn(
                      "create tipset failed, blocks have same ticket ({} {})",
                      address::encodeToString(b1.miner),
                      address::encodeToString(b2.miner));
                  return cid1.toPrettyString("") < cid2.toPrettyString("");
                }
                return *t1 < *t2;
              });

    Tipset ts{};
    std::vector<CID> cids;

    cids.reserve(items.size());
    ts.blks.reserve(items.size());

    for (auto &[b, c] : items) {
      ts.blks.push_back(std::move(b));
      cids.push_back(std::move(c));
    }

    OUTCOME_TRY(key, TipsetKey::create(std::move(cids)));
    ts.key = std::move(key);

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

  outcome::result<void> Tipset::visitMessages(
      IpldPtr ipld, const MessageVisitor::Visitor &visitor) const {
    MessageVisitor message_visitor{ipld};
    for (auto &block : blks) {
      OUTCOME_TRY(message_visitor.visit(block, visitor));
    }
    return outcome::success();
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
  outcome::result<fc::primitives::tipset::Tipset>
  decode<fc::primitives::tipset::Tipset>(gsl::span<const uint8_t> input) {
    using namespace fc::primitives::tipset;

    OUTCOME_TRY(decoded, decode<TipsetDecodeCandidate>(input));
    if (decoded.blks.empty() && decoded.height != 0) {
      return TipsetError::MISMATCHING_HEIGHTS;
    }
    OUTCOME_TRY(tipset, Tipset::create(std::move(decoded.blks)));
    if (tipset.key.cids() != decoded.cids) {
      return TipsetError::BLOCK_ORDER_FAILURE;
    }
    return std::move(tipset);
  }

}  // namespace fc::codec::cbor
