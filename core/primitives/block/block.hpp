/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/assert.hpp>
#include <boost/optional.hpp>

#include "adt/array.hpp"
#include "cbor_blake/cid_block.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "crypto/signature/signature.hpp"
#include "drand/messages.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/big_int.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/message/message.hpp"

namespace fc::primitives::block {
  using crypto::signature::Signature;
  using drand::BeaconEntry;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::address::Address;
  using primitives::sector::PoStProof;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  struct Ticket {
    Bytes bytes;
  };
  inline bool operator==(const Ticket &lhs, const Ticket &rhs) {
    return lhs.bytes == rhs.bytes;
  }
  CBOR_TUPLE(Ticket, bytes)

  JSON_ENCODE(Ticket) {
    using codec::json::Value;
    using codec::json::Set;
    Value j{rapidjson::kObjectType};
    Set(j, "VRFProof", gsl::make_span(v.bytes), allocator);
    return j;
  }

  JSON_DECODE(Ticket) {
    using codec::json::Get;
    codec::json::decode(v.bytes, Get(j, "VRFProof"));
  }

  struct ElectionProof {
    int64_t win_count = 0;
    Bytes vrf_proof;
  };
  inline bool operator==(const ElectionProof &lhs, const ElectionProof &rhs) {
    return lhs.vrf_proof == rhs.vrf_proof;
  }
  CBOR_TUPLE(ElectionProof, win_count, vrf_proof)

  struct BlockTemplate {
    Address miner;
    std::vector<CbCid> parents;
    boost::optional<Ticket> ticket;
    ElectionProof election_proof;
    std::vector<BeaconEntry> beacon_entries;
    std::vector<SignedMessage> messages;
    ChainEpoch height{};
    uint64_t timestamp{};
    std::vector<PoStProof> win_post_proof;
  };

  struct BlockHeader {
    Address miner;
    boost::optional<Ticket> ticket;
    ElectionProof election_proof;
    std::vector<BeaconEntry> beacon_entries;
    std::vector<PoStProof> win_post_proof;
    BlockParentCbCids parents;
    BigInt parent_weight;
    ChainEpoch height{};
    CID parent_state_root;
    CID parent_message_receipts;
    CID messages;
    boost::optional<Signature> bls_aggregate;
    uint64_t timestamp{};
    boost::optional<Signature> block_sig;
    uint64_t fork_signaling{};
    BigInt parent_base_fee;
  };

  struct MsgMeta {
    adt::Array<CID> bls_messages;
    adt::Array<CID> secp_messages;
  };
  CBOR_TUPLE(MsgMeta, bls_messages, secp_messages)

  struct BlockWithMessages {
    BlockHeader header;
    std::vector<UnsignedMessage> bls_messages;
    std::vector<SignedMessage> secp_messages;
  };

  struct BlockWithCids {
    BlockHeader header;
    std::vector<CID> bls_messages;
    std::vector<CID> secp_messages;
  };
  CBOR_TUPLE(BlockWithCids, header, bls_messages, secp_messages)

  inline bool operator==(const BlockHeader &lhs, const BlockHeader &rhs) {
    return lhs.miner == rhs.miner && lhs.ticket == rhs.ticket
           && lhs.election_proof == rhs.election_proof
           && lhs.beacon_entries == rhs.beacon_entries
           && lhs.win_post_proof == rhs.win_post_proof
           && lhs.parents == rhs.parents
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
             election_proof,
             beacon_entries,
             win_post_proof,
             parents,
             parent_weight,
             height,
             parent_state_root,
             parent_message_receipts,
             messages,
             bls_aggregate,
             timestamp,
             block_sig,
             fork_signaling,
             parent_base_fee)
}  // namespace fc::primitives::block

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<primitives::block::MsgMeta> {
    template <typename Visitor>
    static void call(primitives::block::MsgMeta &meta, const Visitor &visit) {
      visit(meta.bls_messages);
      visit(meta.secp_messages);
    }
  };
}  // namespace fc::cbor_blake
