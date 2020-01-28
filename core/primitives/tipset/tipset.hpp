/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP

#include <boost/optional.hpp>
#include "common/outcome.hpp"
#include "common/outcome_throw.hpp"
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
    TipsetKey makeKey() const;

    /**
     * @return key made of parents
     */
    TipsetKey getParents();

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
     * @return parent state root
     */
    CID getParentStateRoot() const;

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

  bool operator==(const Tipset &lhs, const Tipset &rhs);

  /**
   * @brief cbor-encodes Tipset instance
   * @tparam Stream cbor-encoder stream type
   * @param s stream reference
   * @param tipset Tipset instance
   * @return stream reference
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Tipset &tipset) noexcept {
    return s << (s.list() << tipset.cids << tipset.blks << tipset.height);
  }

  /**
   * @brief cbor-decodes Tipset instance
   * @tparam Stream cbor-decoder stream type
   * @param s stream reference
   * @param tipset Tipset instance reference to decode into
   * @return stream reference
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Tipset &tipset) {
    std::vector<CID> cids;
    std::vector<block::BlockHeader> blks;

    s.list() >> cids >> blks >> tipset.height;
    tipset.cids = std::move(cids);
    tipset.blks = std::move(blks);
    return s;
  }

}  // namespace fc::primitives::tipset

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
