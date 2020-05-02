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
  };
}

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::primitives::tipset, TipsetError);

namespace fc::primitives::tipset {
  /**
   * @struct Tipset implemented according to
   * https://github.com/filecoin-project/lotus/blob/6e94377469e49fa4e643f9204b6f46ef3cb3bf04/chain/types/tipset.go#L18
   */
  struct Tipset {
    static outcome::result<Tipset> create(
        std::vector<block::BlockHeader> blocks);

    /**
     * @brief makes key of cids
     */
    outcome::result<TipsetKey> makeKey() const;

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
    CID getParentStateRoot() const;

    inline CID getParentMessageReceipts() const {
      return blks[0].parent_message_receipts;
    }

    /**
     * @return parent weight
     */
    BigInt getParentWeight() const;

    /**
     * @brief checks whether tipset contains cid
     * @param cid content identifier to look for
     * @return true if contains, false otherwise
     */
    bool contains(const CID &cid) const;

    std::vector<CID> cids;                 ///< cids
    std::vector<block::BlockHeader> blks;  ///< block headers
    uint64_t height{};                     ///< height
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

  CBOR_TUPLE(Tipset, cids, blks, height)

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
}  // namespace fc::primitives::tipset

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
