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
      return "Need to have at least one block to create tipset";
    case TipsetError::MISMATCHING_HEIGHTS:
      return "Cannot create tipset, mismatching blocks heights";
    case TipsetError::MISMATCHING_PARENTS:
      return "Cannot create tipset, mismatching block parents";
    case TipsetError::TICKET_HAS_NO_VALUE:
      return "An optional ticket is not initialized";
  }
  return "Unknown tipset error";
}

namespace fc::primitives::tipset {

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
    ts.height = height0;
    ts.cids.reserve(items.size());
    ts.blks.reserve(items.size());

    for (auto &[b, c] : items) {
      ts.blks.push_back(std::move(b));
      ts.cids.push_back(std::move(c));
    }

    return ts;
  }

  TipsetKey Tipset::getParents() {
    return TipsetKey{blks[0].parents};
  }

  TipsetKey Tipset::makeKey() const {
    return TipsetKey{cids};
  }

  outcome::result<boost::optional<ticket::Ticket>> Tipset::getMinTicket()
      const {
    OUTCOME_TRY(block, getMinTicketBlock());
    return block.get().ticket;
  }

  uint64_t Tipset::getMinTimestamp() const {
    return std::min_element(blks.begin(),
                            blks.end(),
                            [](const auto &b1, const auto &b2) -> bool {
                              return b1.timestamp < b2.timestamp;
                            })
        ->timestamp;
  }

  outcome::result<std::reference_wrapper<const block::BlockHeader>>
  Tipset::getMinTicketBlock() const {
    std::reference_wrapper<const block::BlockHeader> block = blks[0];
    if (!block.get().ticket.has_value()) {
      return TipsetError::TICKET_HAS_NO_VALUE;
    }
    for (auto &b : blks) {
      if (!b.ticket.has_value()) {
        return TipsetError::TICKET_HAS_NO_VALUE;
      }
      if (b.ticket < block.get().ticket) {
        block = b;
      }
    }
    return block;
  }

  CID Tipset::getParentStateRoot() const {
    return blks[0].parent_state_root;
  }

  BigInt Tipset::getParentWeight() const {
    return blks[0].parent_weight;
  }

  bool Tipset::contains(const CID &cid) const {
    return std::find(cids.begin(), cids.end(), cid) != std::end(cids);
  }

  /**
   * @brief compares two Tipset instances
   * @param lhs first tipset
   * @param rhs second tipset
   * @return true if equal, false otherwise
   */
  bool operator==(const Tipset &lhs, const Tipset &rhs) {
    if (lhs.blks.size() != rhs.blks.size()) return false;
    return std::equal(lhs.blks.begin(), lhs.blks.end(), rhs.blks.begin());
  }

}  // namespace fc::primitives::tipset
