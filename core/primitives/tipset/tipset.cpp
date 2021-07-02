/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset.hpp"

#include "common/logger.hpp"
#include "const.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "vm/message/message.hpp"

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
    case TipsetError::kTicketsCollision:
      return "Duplicate tickets in tipset";
    case TipsetError::kBlockOrderFailure:
      return "Wrong order of blocks in tipset";
    case TipsetError::kMinerAlreadyExists:
      return "Same miner already in tipset";
    case TipsetError::kNoBeacons:
      return "No beacons in chain";
  }
  return "Unknown tipset error";
}

namespace fc::primitives::tipset {

  namespace {

    outcome::result<Hash256> ticketHash(const block::BlockHeader &hdr) {
      if (!hdr.ticket.has_value()) {
        if (hdr.height == 0) {
          // Genesis block may not have ticket
          return Hash256{};
        }
        return TipsetError::kTicketHasNoValue;
      }

      return crypto::blake2b::blake2b_256(hdr.ticket.value().bytes);
    }

  }  // namespace
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  outcome::result<void> MessageVisitor::visit(const BlockHeader &block,
                                              const Visitor &visitor) {
    auto onMessage = [&](auto bls, auto &cid) -> outcome::result<void> {
      if (visited.insert(cid).second) {
        SignedMessage smsg;
        auto &msg{smsg.message};
        if (load) {
          if (bls) {
            OUTCOME_TRYA(msg, ipld->getCbor<UnsignedMessage>(cid));
          } else {
            OUTCOME_TRYA(smsg, ipld->getCbor<SignedMessage>(cid));
          }
        }
        if (nonce) {
          auto it{nonces.find(msg.from)};
          if (it == nonces.end()) {
            it = nonces.emplace(msg.from, msg.nonce).first;
          }
          if (msg.nonce != it->second) {
            return outcome::success();
          }
          ++it->second;
        }
        OUTCOME_TRY(visitor(index,
                            bls,
                            cid,
                            load && !bls ? &smsg : nullptr,
                            load ? &msg : nullptr));
        ++index;
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

  outcome::result<void> TipsetCreator::canExpandTipset(
      const block::BlockHeader &hdr) const {
    if (blks_.empty()) {
      return outcome::success();
    }

    if (hdr.height > 0 && !hdr.ticket.has_value()) {
      return TipsetError::kTicketHasNoValue;
    }

    const auto &first_block = blks_[0];

    if (hdr.height != first_block.height) {
      return TipsetError::kMismatchingHeights;
    }

    if (hdr.parents != first_block.parents) {
      return TipsetError::kMismatchingParents;
    }

    for (const auto &b : blks_) {
      if (b.miner == hdr.miner) {
        return TipsetError::kMinerAlreadyExists;
      }
    }

    return outcome::success();
  }

  outcome::result<CbCid> TipsetCreator::expandTipset(block::BlockHeader hdr) {
    OUTCOME_TRY(cbor, codec::cbor::encode(hdr));
    auto cid{CbCid::hash(cbor)};
    OUTCOME_TRY(expandTipset(cid, std::move(hdr)));
    return std::move(cid);
  }

  outcome::result<void> TipsetCreator::expandTipset(CbCid cid,
                                                    block::BlockHeader hdr) {
    // must be called prior to expand()
    assert(canExpandTipset(hdr));

    constexpr auto kReserveSize = 5;

    if (blks_.empty()) {
      blks_.reserve(kReserveSize);
      cids_.reserve(kReserveSize);
      ticket_hashes_.reserve(kReserveSize);
      OUTCOME_TRY(hash, ticketHash(hdr));
      ticket_hashes_.emplace_back(std::move(hash));
      cids_.emplace_back(std::move(cid));
      blks_.emplace_back(std::move(hdr));
      return outcome::success();
    }

    OUTCOME_TRY(ticket_hash, ticketHash(hdr));
    size_t idx = 0;
    auto sz = ticket_hashes_.size();
    for (; idx != sz; ++idx) {
      const auto &h = ticket_hashes_[idx];
      if (ticket_hash < h) {
        break;
      }
      if (ticket_hash == h) {
        if (cid >= cids_[idx]) {
          continue;
        } else {
          break;
        }
      }
    }

    if (idx == sz) {
      // most likely they come in proper order
      blks_.push_back(std::move(hdr));
      cids_.push_back(std::move(cid));
      ticket_hashes_.push_back(std::move(ticket_hash));
    } else {
      blks_.insert(blks_.begin() + idx, std::move(hdr));
      cids_.insert(cids_.begin() + idx, std::move(cid));
      ticket_hashes_.insert(ticket_hashes_.begin() + idx,
                            std::move(ticket_hash));
    }

    return outcome::success();
  }

  TipsetCPtr TipsetCreator::getTipset(bool clear) {
    if (blks_.empty()) {
      return std::make_shared<Tipset>();
    }
    if (clear) {
      ticket_hashes_.clear();
      return std::make_shared<Tipset>(std::move(cids_), std::move(blks_));
    }
    return std::make_shared<Tipset>(cids_, blks_);
  }

  void TipsetCreator::clear() {
    blks_.clear();
    cids_.clear();
    ticket_hashes_.clear();
  }

  Height TipsetCreator::height() const {
    return blks_.empty() ? 0 : blks_[0].height;
  }

  TipsetKey TipsetCreator::key() const {
    return cids_;
  }

  outcome::result<TipsetCPtr> Tipset::create(const TipsetHash &hash,
                                             BlocksFromNetwork blocks) {
    TipsetCreator creator;

    for (auto &b : blocks) {
      if (!b.has_value()) {
        return TipsetError::kNoBlocks;
      }

      auto &hdr = b.value();
      OUTCOME_TRY(creator.canExpandTipset(hdr));
      OUTCOME_TRY(creator.expandTipset(std::move(hdr)));
    }

    TipsetCPtr tipset = creator.getTipset(true);
    if (tipset->key.hash() != hash) {
      return TipsetError::kBlockOrderFailure;
    }

    return std::move(tipset);
  }

  outcome::result<TipsetCPtr> Tipset::create(
      std::vector<block::BlockHeader> blocks) {
    if (blocks.empty()) {
      return TipsetError::kNoBlocks;
    }

    TipsetCreator creator;

    for (auto &hdr : blocks) {
      OUTCOME_TRY(creator.canExpandTipset(hdr));
      OUTCOME_TRY(creator.expandTipset(std::move(hdr)));
    }

    return creator.getTipset(true);
  }

  Tipset::Tipset(const TipsetKey &key, std::vector<block::BlockHeader> blks)
      : key(std::move(key)), blks(std::move(blks)) {}

  outcome::result<void> Tipset::visitMessages(
      MessageVisitor message_visitor,
      const MessageVisitor::Visitor &visitor) const {
    for (auto &block : blks) {
      OUTCOME_TRY(message_visitor.visit(block, visitor));
    }
    return outcome::success();
  }

  outcome::result<BigInt> Tipset::nextBaseFee(IpldPtr ipld) const {
    if (kUpgradeBreezeHeight >= 0 && epoch() > kUpgradeBreezeHeight
        && epoch() < kUpgradeBreezeHeight + kBreezeGasTampingDuration) {
      return 100;
    }
    GasAmount gas_limit{};
    OUTCOME_TRY(
        visitMessages({ipld, false, true},
                      [&](auto, auto bls, auto &cid, auto *smsg, auto *msg)
                          -> outcome::result<void> {
                        gas_limit += msg->gas_limit;
                        return outcome::success();
                      }));
    auto delta{std::max<GasAmount>(
        -kBlockGasTarget,
        std::min<GasAmount>(
            kBlockGasTarget,
            kPackingEfficiencyDenom * gas_limit
                    / ((int64_t)blks.size() * kPackingEfficiencyNum)
                - kBlockGasTarget))};
    auto base{getParentBaseFee()};
    return std::max<BigInt>(kMinimumBaseFee,
                            base
                                + bigdiv(bigdiv(base * delta, kBlockGasTarget),
                                         kBaseFeeMaxChangeDenom));
  }

  TipsetKey Tipset::getParents() const {
    assert(!blks.empty());
    return blks[0].parents;
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
    assert(!blks.empty());
    // i believe that Tipset::create sorts them
    return blks[0];
  }

  const CID &Tipset::getParentStateRoot() const {
    assert(!blks.empty());
    return blks[0].parent_state_root;
  }

  const BigInt &Tipset::getParentWeight() const {
    assert(!blks.empty());
    return blks[0].parent_weight;
  }

  const CID &Tipset::getParentMessageReceipts() const {
    assert(!blks.empty());
    return blks[0].parent_message_receipts;
  }

  Height Tipset::height() const {
    return blks.empty() ? 0 : blks[0].height;
  }

  ChainEpoch Tipset::epoch() const {
    return height();
  }

  const BigInt &Tipset::getParentBaseFee() const {
    assert(!blks.empty());
    return blks[0].parent_base_fee;
  }

  bool operator==(const Tipset &lhs, const Tipset &rhs) {
    return lhs.blks == rhs.blks;
  }
}  // namespace fc::primitives::tipset
