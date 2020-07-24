/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP

#include <boost/optional.hpp>
#include "common/outcome.hpp"
#include "primitives/block/block.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/ticket/ticket.hpp"
#include "primitives/tipset/tipset_key.hpp"

namespace fc::primitives::tipset {

  enum class TipsetError : int {
    NO_BLOCKS = 1,        // need to have at least one block to create tipset
    MISMATCHING_HEIGHTS,  // cannot create tipset, mismatching blocks heights
    MISMATCHING_PARENTS,  // cannot create tipset, mismatching block parents
    TICKET_HAS_NO_VALUE,  // optional ticket is not initialized
    TICKETS_COLLISION,    // duplicate tickets in tipset
    BLOCK_ORDER_FAILURE,  // wrong order of blocks
  };
}

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::primitives::tipset, TipsetError);

namespace fc::primitives::tipset {

  using block::BlockHeader;

  struct MessageVisitor {
    using Visitor =
        std::function<outcome::result<void>(size_t, bool bls, const CID &)>;
    outcome::result<void> visit(const BlockHeader &block,
                                const Visitor &visitor);
    IpldPtr ipld;
    std::set<CID> visited{};
  };

  struct Tipset {
    using BlocksAvailable = std::vector<boost::optional<block::BlockHeader>>;

    /// Creates tipset from loaded blocks, every block must have value,
    /// hashes must match
    static outcome::result<Tipset> create(const TipsetHash &hash,
                                          BlocksAvailable blocks);

    static outcome::result<Tipset> create(
        std::vector<block::BlockHeader> blocks);

    static outcome::result<Tipset> load(Ipld &ipld,
                                        const std::vector<CID> &cids);

    static outcome::result<Tipset> loadGenesis(Ipld &ipld,
                                        const CID &cid);

    outcome::result<Tipset> loadParent(Ipld &ipld) const;

    outcome::result<void> visitMessages(
        IpldPtr ipld, const MessageVisitor::Visitor &visitor) const;

    /**
     * @return key made of parents
     */
    outcome::result<TipsetKey> getParents() const;

    /**
     * @return min timestamp
     */
    uint64_t getMinTimestamp() const;

    /**
     * @return min ticket block
     */
    const block::BlockHeader &getMinTicketBlock() const;

    /**
     * @return parent state root
     */
    const CID &getParentStateRoot() const;

    const CID &getParentMessageReceipts() const;

    uint64_t height() const;

    /**
     * @return parent weight
     */
    const BigInt &getParentWeight() const;

    /**
     * @brief checks whether tipset contains cid
     * @param cid content identifier to look for
     * @return true if contains, false otherwise
     */
    bool contains(const CID &cid) const;

    TipsetKey key;
    std::vector<block::BlockHeader> blks;  ///< block headers
  };

  /**
   * @brief compares two Tipset instances
   * @param lhs first tipset
   * @param rhs second tipset
   * @return true if equal, false otherwise
   */
  bool operator==(const Tipset &l, const Tipset &r);

  /**
   * @brief compares two Tipset instances
   * @param lhs first tipset
   * @param rhs second tipset
   * @return false if equal, true otherwise
   */
  bool operator!=(const Tipset &l, const Tipset &r);

  CBOR_ENCODE_TUPLE(Tipset, key.cids(), blks, height())

  /**
   * @brief change type
   */
  enum class HeadChangeType : int { REVERT, APPLY, CURRENT };

  /**
   * @struct HeadChange represents atomic chain change
   */
  struct HeadChange {
    HeadChangeType type;
    Tipset value;
  };

  class TipsetCreator {
   public:
    /// returns success if the tipset created can be expanded with this block
    outcome::result<void> canExpandTipset(const block::BlockHeader &hdr) const;

    outcome::result<void> expandTipset(block::BlockHeader hdr);

    outcome::result<void> expandTipset(CID cid, block::BlockHeader hdr);

    Tipset getTipset(bool clear);

    void clear();

    uint64_t height() const;

   private:
    std::vector<block::BlockHeader> blks_;
    std::vector<CID> cids_;
    std::vector<std::array<uint8_t, 32>> ticket_hashes_;
  };

}  // namespace fc::primitives::tipset

namespace fc::codec::cbor {

  template <>
  outcome::result<fc::primitives::tipset::Tipset>
  decode<fc::primitives::tipset::Tipset>(gsl::span<const uint8_t> input);

}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
