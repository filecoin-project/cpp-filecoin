/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/block/block.hpp"
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
  using block::Address;
  using block::BeaconEntry;
  using block::BlockHeader;
  using common::Hash256;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  using Height = uint64_t;

  struct MessageVisitor {
    using Visitor = std::function<outcome::result<void>(size_t,
                                                        bool bls,
                                                        const CID &,
                                                        SignedMessage *smsg,
                                                        UnsignedMessage *msg)>;
    MessageVisitor(IpldPtr ipld, bool nonce, bool load)
        : ipld{ipld}, nonce{nonce}, load{load || nonce} {}
    outcome::result<void> visit(const BlockHeader &block,
                                const Visitor &visitor);
    IpldPtr ipld;
    bool nonce{};
    bool load{};
    std::set<CID> visited;
    std::map<Address, uint64_t> nonces;
    size_t index{};
  };

  struct Tipset;
  using TipsetCPtr = std::shared_ptr<const Tipset>;
  using TsWeak = std::weak_ptr<const Tipset>;

  struct Tipset {
    /// Blocks from network, they may come in improper order
    using BlocksFromNetwork = std::vector<boost::optional<block::BlockHeader>>;

    /// Creates tipset from loaded blocks, every block must have value,
    /// hashes must match
    static outcome::result<TipsetCPtr> create(const TipsetHash &hash,
                                              BlocksFromNetwork blocks);

    static outcome::result<TipsetCPtr> create(
        std::vector<block::BlockHeader> blocks);

    Tipset() = default;

    Tipset(const TipsetKey &key, std::vector<block::BlockHeader> blks);

    outcome::result<void> visitMessages(
        MessageVisitor message_visitor,
        const MessageVisitor::Visitor &visitor) const;

    outcome::result<BigInt> nextBaseFee(IpldPtr ipld) const;

    /**
     * @return key made of parents
     */
    TipsetKey getParents() const;

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

    /**
     * @return adt::map root CID of message receipts
     */
    const CID &getParentMessageReceipts() const;

    Height height() const;

    ChainEpoch epoch() const;

    /**
     * @return parent weight
     */
    const BigInt &getParentWeight() const;

    const BigInt &getParentBaseFee() const;

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
  FC_OPERATOR_NOT_EQUAL(Tipset)

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

    outcome::result<CbCid> expandTipset(block::BlockHeader hdr);

    outcome::result<void> expandTipset(CbCid cid, block::BlockHeader hdr);

    TipsetCPtr getTipset(bool clear);

    void clear();

    Height height() const;

    TipsetKey key() const;

   private:
    std::vector<block::BlockHeader> blks_;
    std::vector<CbCid> cids_;
    std::vector<Hash256> ticket_hashes_;
  };

}  // namespace fc::primitives::tipset
