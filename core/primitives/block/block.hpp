/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/optional.hpp>

#include "codec/cbor/streams_annotation.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/big_int.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/ticket/epost_ticket.hpp"
#include "primitives/ticket/epost_ticket_codec.hpp"
#include "primitives/ticket/ticket.hpp"
#include "primitives/ticket/ticket_codec.hpp"

namespace fc::primitives::block {
  using primitives::BigInt;
  using primitives::address::Address;
  using primitives::ticket::EPostProof;
  using primitives::ticket::Ticket;
  // TODO (yuraz) : FIL-142 replace by crypto::signature::Signature
  using Signature = std::vector<uint8_t>;

  struct BlockHeader {
    Address miner;
    boost::optional<Ticket> ticket;
    EPostProof epost_proof;
    std::vector<CID> parents;
    BigInt parent_weight;
    uint64_t height;
    CID parent_state_root;
    CID parent_message_receipts;
    CID messages;
    Signature bls_aggregate;
    uint64_t timestamp;
    boost::optional<Signature> block_sig;
    uint64_t fork_signaling;
  };

  struct MsgMeta {
    CID bls_messages;
    CID secpk_messages;
  };

  inline bool operator==(const BlockHeader &lhs, const BlockHeader &rhs) {
    return lhs.miner == rhs.miner && lhs.ticket == rhs.ticket
           && lhs.epost_proof == rhs.epost_proof && lhs.parents == rhs.parents
           && lhs.parent_weight == rhs.parent_weight && lhs.height == rhs.height
           && lhs.parent_state_root == rhs.parent_state_root
           && lhs.parent_message_receipts == rhs.parent_message_receipts
           && lhs.messages == rhs.messages
           && lhs.bls_aggregate == rhs.bls_aggregate
           && lhs.timestamp == rhs.timestamp && lhs.block_sig == rhs.block_sig
           && lhs.fork_signaling == rhs.fork_signaling;
  }

  CBOR_ENCODE(BlockHeader, block) {
    return s << (s.list() << block.miner << block.ticket << block.epost_proof
                          << block.parents << block.parent_weight
                          << block.height << block.parent_state_root
                          << block.parent_message_receipts << block.messages
                          << block.bls_aggregate << block.timestamp
                          << block.block_sig << block.fork_signaling);
  }

  CBOR_DECODE(BlockHeader, block) {
    s.list() >> block.miner >> block.ticket >> block.epost_proof
        >> block.parents >> block.parent_weight >> block.height
        >> block.parent_state_root >> block.parent_message_receipts
        >> block.messages >> block.bls_aggregate >> block.timestamp
        >> block.block_sig >> block.fork_signaling;
    return s;
  }

  CBOR_ENCODE(MsgMeta, meta) {
    return s << (s.list() << meta.bls_messages << meta.secpk_messages);
  }

  CBOR_DECODE(MsgMeta, meta) {
    s.list() >> meta.bls_messages >> meta.secpk_messages;
    return s;
  }
}  // namespace fc::primitives::block
