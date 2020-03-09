/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_BLOCK_BLOCK_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_BLOCK_BLOCK_HPP

#include <boost/assert.hpp>
#include <boost/optional.hpp>

#include "filecoin/codec/cbor/streams_annotation.hpp"
#include "filecoin/crypto/signature/signature.hpp"
#include "filecoin/primitives/address/address.hpp"
#include "filecoin/primitives/address/address_codec.hpp"
#include "filecoin/primitives/big_int.hpp"
#include "filecoin/primitives/cid/cid.hpp"
#include "filecoin/primitives/ticket/epost_ticket.hpp"
#include "filecoin/primitives/ticket/epost_ticket_codec.hpp"
#include "filecoin/primitives/ticket/ticket.hpp"
#include "filecoin/primitives/ticket/ticket_codec.hpp"
#include "filecoin/storage/ipld/ipld_block_common.hpp"
#include "filecoin/vm/message/message.hpp"

namespace fc::primitives::block {
  using primitives::BigInt;
  using primitives::address::Address;
  using primitives::ticket::EPostProof;
  using primitives::ticket::Ticket;
  // TODO (yuraz) : FIL-142 replace by crypto::signature::Signature
  using Signature = std::vector<uint8_t>;
  using storage::ipld::IPLDBlockCommon;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

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

  struct MsgMeta : public IPLDBlockCommon<CID::Version::V1,
                                        storage::ipld::HashType::blake2b_256,
                                        storage::ipld::ContentType::DAG_CBOR> {
    CID bls_messages;
    CID secpk_messages;

    outcome::result<std::vector<uint8_t>> getBlockContent() const override {
      return codec::cbor::encode<MsgMeta>(*this);
    }
  };

  struct Block {
    BlockHeader header;
    std::vector<UnsignedMessage> bls_messages;
    std::vector<SignedMessage> secp_messages;
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

  CBOR_TUPLE(MsgMeta, bls_messages, secpk_messages)
}  // namespace fc::primitives::block

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_BLOCK_BLOCK_HPP
