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
    kNoBlocks = 1,        // need to have at least one block to create tipset
    kMismatchingHeights,  // cannot create tipset, mismatching blocks heights
    kMismatchingParents,  // cannot create tipset, mismatching block parents
    kTicketHasNoValue,    // optional ticket is not initialized
    kTicketsCollision,    // duplicate tickets in tipset
    kBlockOrderFailure,   // wrong order of blocks
    kMinerAlreadyExists,  // miner already in tipset
    kNoBeacons,
  };
}

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::primitives::tipset, TipsetError);

namespace fc::primitives::tipset {
  using block::BeaconEntry;
  using block::BlockHeader;
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;

  struct MessageVisitor {
    using Visitor =
        std::function<outcome::result<void>(size_t, bool bls, const CID &)>;
    outcome::result<void> visit(const BlockHeader &block,
                                const Visitor &visitor);
    IpldPtr ipld;
    std::set<CID> visited{};
  };

  struct Tipset;
  using TipsetCPtr = std::shared_ptr<const Tipset>;

  struct Tipset {
    /// Blocks from network, they may come in improper order
    using BlocksFromNetwork = std::vector<boost::optional<block::BlockHeader>>;

    /// Creates tipset from loaded blocks, every block must have value,
    /// hashes must match
    static outcome::result<TipsetCPtr> create(const TipsetHash &hash,
                                              BlocksFromNetwork blocks);

    static outcome::result<TipsetCPtr> create(
        std::vector<block::BlockHeader> blocks);

    static outcome::result<TipsetCPtr> load(Ipld &ipld,
                                            const std::vector<CID> &cids);

    static outcome::result<TipsetCPtr> loadGenesis(Ipld &ipld, const CID &cid);

    outcome::result<TipsetCPtr> loadParent(Ipld &ipld) const;

    outcome::result<BeaconEntry> latestBeacon(Ipld &ipld) const;

    outcome::result<void> visitMessages(
        IpldPtr ipld, const MessageVisitor::Visitor &visitor) const;

    outcome::result<Randomness> randomness(
        Ipld &ipld,
        DomainSeparationTag tag,
        ChainEpoch round,
        gsl::span<const uint8_t> entropy) const;

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
     * @brief checks whether tipset contains block by cid
     * @param cid content identifier to look for
     * @return true if contains, false otherwise
     */
    bool contains(const CID &cid) const;

    Tipset() = default;

    Tipset(TipsetKey _key, std::vector<block::BlockHeader> _blks)
        : key(std::move(_key)), blks(std::move(_blks)) {}

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
    TipsetCPtr value;
  };

  class TipsetCreator {
   public:
    /// returns success if the tipset created can be expanded with this block
    outcome::result<void> canExpandTipset(const block::BlockHeader &hdr) const;

    outcome::result<CID> expandTipset(block::BlockHeader hdr);

    outcome::result<void> expandTipset(CID cid, block::BlockHeader hdr);

    TipsetCPtr getTipset(bool clear);

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
  outcome::result<fc::primitives::tipset::TipsetCPtr>
  decode<fc::primitives::tipset::TipsetCPtr>(gsl::span<const uint8_t> input);

  template <> inline
  outcome::result<common::Buffer> encode<fc::primitives::tipset::TipsetCPtr>(
      const fc::primitives::tipset::TipsetCPtr &ts) {
    assert(ts);
    return encode<fc::primitives::tipset::Tipset>(*ts);
  }

}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
