/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_BLOCK_BLOCK_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_BLOCK_BLOCK_HPP

#include <boost/assert.hpp>
#include <boost/optional.hpp>

#include "adt/array.hpp"
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
#include "vm/message/message.hpp"

namespace fc::primitives::block {
  using crypto::signature::Signature;
  using primitives::BigInt;
  using primitives::address::Address;
  using primitives::ticket::EPostProof;
  using primitives::ticket::Ticket;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using Ipld = storage::ipfs::IpfsDatastore;

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
    void load(std::shared_ptr<Ipld> ipld) {
      bls_messages.load(ipld);
      secp_messages.load(ipld);
    }
    outcome::result<void> flush() {
      OUTCOME_TRY(bls_messages.flush());
      OUTCOME_TRY(secp_messages.flush());
      return outcome::success();
    }

    adt::Array<CID> bls_messages;
    adt::Array<CID> secp_messages;
  };
  CBOR_TUPLE(MsgMeta, bls_messages, secp_messages)

  struct Block {
    BlockHeader header;
    std::vector<UnsignedMessage> bls_messages;
    std::vector<SignedMessage> secp_messages;
  };

  struct BlockMsg {
    BlockHeader header;
    std::vector<CID> bls_messages;
    std::vector<CID> secp_messages;
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

  CBOR_TUPLE(BlockHeader,
             miner,
             ticket,
             epost_proof,
             parents,
             parent_weight,
             height,
             parent_state_root,
             parent_message_receipts,
             messages,
             bls_aggregate,
             timestamp,
             block_sig,
             fork_signaling)
}  // namespace fc::primitives::block

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_BLOCK_BLOCK_HPP
