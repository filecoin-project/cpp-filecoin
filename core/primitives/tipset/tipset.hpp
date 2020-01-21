/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP

#include "primitives/block/block.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/ticket/ticket.hpp"
#include "primitives/tipset/tipset_key.hpp"

#include <boost/optional.hpp>

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
  class Tipset {
    friend bool operator==(const Tipset &lhs, const Tipset &rhs);

   public:
    static outcome::result<Tipset> create(
        std::vector<block::BlockHeader> blocks);

    /**
     * @return span of cids
     */
    gsl::span<const CID> getCids() const;

    /**
     * @brief makes key of cids
     */
    outcome::result<TipsetKey> makeKey() const;

    /**
     * @return height
     */
    uint64_t getHeight() const;

    /**
     * @return key made of parents
     */
    outcome::result<TipsetKey> getParents();

    /**
     * @return span of blocks
     */
    gsl::span<const block::BlockHeader> getBlocks() const;

    /**
     * @return optional min ticket or error
     */
    outcome::result<boost::optional<ticket::Ticket>> getMinTicket() const;

    /**
     * @return min timestamp
     */
    uint64_t getMinTimestamp() const;

    /**
     * @return min ticket block
     */
    outcome::result<std::reference_wrapper<const block::BlockHeader>>
    getMinTicketBlock() const;

    /**
     * @return parent state
     */
    CID getParentState() const;

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

   private:
    std::vector<CID> cids_;                 ///< cids
    std::vector<block::BlockHeader> blks_;  ///< block headers
    uint64_t height_{};                       ///< height
  };

  bool operator==(const Tipset &lhs, const Tipset &rhs);

}  // namespace fc::primitives::tipset

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
