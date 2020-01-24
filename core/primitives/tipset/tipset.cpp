/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset.hpp"

#include "codec/cbor/cbor_cid.hpp"
#include "common/logger.hpp"
#include "primitives/address/address_codec.hpp"

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
      return "optional ticket is not initialized";
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
      OUTCOME_TRY(cid, codec::cbor::getCidOfCbor(block));
      // need to ensure that all cids are calculated before sort,
      // since it will terminate program in case of exception
      items.emplace_back(std::make_pair(std::move(block), std::move(cid)));
    }

    // if an exception thrown from std::sort, program is terminated
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
    ts.height_ = height0;
    ts.cids_.reserve(items.size());
    ts.blks_.reserve(items.size());

    for (auto &[b, c] : items) {
      ts.blks_.push_back(std::move(b));
      ts.cids_.push_back(std::move(c));
    }

    return ts;
  }

  gsl::span<const CID> Tipset::getCids() const {
    return cids_;
  }

  gsl::span<const block::BlockHeader> Tipset::getBlocks() const {
    return blks_;
  }

  uint64_t Tipset::getHeight() const {
    return height_;
  }

  TipsetKey Tipset::getParents() {
    return TipsetKey{blks_[0].parents};
  }

  TipsetKey Tipset::makeKey() const {
    return TipsetKey{cids_};
  }

  outcome::result<boost::optional<ticket::Ticket>> Tipset::getMinTicket()
      const {
    OUTCOME_TRY(block, getMinTicketBlock());
    return block.get().ticket;
  }

  uint64_t Tipset::getMinTimestamp() const {
    auto timestamp = blks_[0].timestamp;
    for (auto &b : blks_) {
      if (b.timestamp < timestamp) {
        timestamp = b.timestamp;
      }
    }
    return timestamp;
  }

  outcome::result<std::reference_wrapper<const block::BlockHeader>>
  Tipset::getMinTicketBlock() const {
    std::reference_wrapper<const block::BlockHeader> block = blks_[0];
    if (!block.get().ticket.has_value()) {
      return TipsetError::TICKET_HAS_NO_VALUE;
    }
    for (auto &b : blks_) {
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
    return blks_[0].parent_state_root;
  }

  BigInt Tipset::getParentWeight() const {
    return blks_[0].parent_weight;
  }

  bool Tipset::contains(const CID &cid) const {
    for (auto &c : cids_) {
      if (cid == c) {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief compares two Tipset instances
   * @param lhs first tipset
   * @param rhs second tipset
   * @return true if equal, false otherwise
   */
  bool operator==(const Tipset &lhs, const Tipset &rhs) {
    if (lhs.blks_.size() != rhs.blks_.size()) return false;
    for (size_t i = 0; i < lhs.blks_.size(); ++i) {
      if (!(lhs.blks_[i] == rhs.blks_[i])) {
        return false;
      }
    }
    return true;
  }

}  // namespace fc::primitives::tipset
