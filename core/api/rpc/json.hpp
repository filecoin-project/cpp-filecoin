/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @note This file collects all the encodings that are used in the serialization
 * of structures for the server. If you want to use any structures outside the
 * server, then you can freely take them there, and write #include here.
 */

#include "codec/json/coding.hpp"

#include "api/full_node/node_api.hpp"
#include "api/rpc/rpc.hpp"
#include "api/storage_miner/storage_api.hpp"
#include "api/types/key_info.hpp"
#include "api/types/ledger_key_info.hpp"
#include "api/worker_api.hpp"
#include "common/enum.hpp"
#include "common/libp2p/peer/cbor_peer_info.hpp"
#include "miner/main/type.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "primitives/json_types.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"
#include "vm/actor/builtin/types/market/v0/deal_proposal.hpp"
#include "vm/actor/builtin/types/miner/miner_info.hpp"

namespace fc::common {
  using markets::storage::StorageDealStatus;

  template <>
  inline StorageDealStatus kDefaultT<StorageDealStatus>() {
    return StorageDealStatus::STORAGE_DEAL_UNKNOWN;
  }
}  // namespace fc::common

namespace boost::multiprecision {
  using fc::codec::json::AsString;
  using fc::codec::json::encode;

  JSON_ENCODE(cpp_int) {
    return encode(boost::lexical_cast<std::string>(v), allocator);
  }

  JSON_DECODE(cpp_int) {
    v = cpp_int{AsString(j)};
  }
}  // namespace boost::multiprecision

namespace libp2p {
  using fc::codec::json::AsString;
  using fc::codec::json::Get;
  using fc::codec::json::Set;
  using fc::codec::json::Value;

  namespace multi {
    using fc::codec::json::encode;

    JSON_ENCODE(Multiaddress) {
      return encode(v.getStringAddress(), allocator);
    }

    JSON_DECODE(Multiaddress) {
      OUTCOME_EXCEPT(_v, Multiaddress::create(AsString(j)));
      v = std::move(_v);
    }
  }  // namespace multi

  namespace peer {
    using fc::codec::json::encode;

    JSON_ENCODE(PeerId) {
      return encode(v.toBase58(), allocator);
    }

    JSON_DECODE(PeerId) {
      OUTCOME_EXCEPT(id, PeerId::fromBase58(AsString(j)));
      v = std::move(id);
    }

    JSON_ENCODE(PeerInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "ID", v.id, allocator);
      Set(j, "Addrs", v.addresses, allocator);
      return j;
    }

    JSON_DECODE(PeerInfo) {
      Get(j, "ID", v.id);
      Get(j, "Addrs", v.addresses);
    }
  }  // namespace peer
}  // namespace libp2p

namespace fc {
  using codec::json::AsString;
  using codec::json::encode;
  using codec::json::Get;
  using codec::json::innerDecode;
  using codec::json::JsonError;
  using codec::json::Set;
  using codec::json::Value;

  JSON_ENCODE(CID) {
    OUTCOME_EXCEPT(str, v.toString());
    Value j{rapidjson::kObjectType};
    Set(j, "/", str, allocator);
    return j;
  }

  JSON_DECODE(CID) {
    OUTCOME_EXCEPT(cid, CID::fromString(AsString(Get(j, "/"))));
    v = std::move(cid);
  }

  JSON_ENCODE(CbCid) {
    return encode(CID{v}, allocator);
  }

  JSON_DECODE(CbCid) {
    if (auto cid{asBlake(innerDecode<CID>(j))}) {
      v = *cid;
    } else {
      outcome::raise(JsonError::kWrongType);
    }
  }

  JSON_ENCODE(BlockParentCbCids) {
    if (v.mainnet_genesis) {
      static const std::vector<CID> mainnet{
          CID::fromBytes(kMainnetGenesisBlockParent).value()};
      return encode(mainnet, allocator);
    }
    return encode<std::vector<CbCid>>(v, allocator);
  }

  JSON_DECODE(BlockParentCbCids) {
    const auto cids{innerDecode<std::vector<CID>>(j)};
    static const std::vector<CID> mainnet{
        CID::fromBytes(kMainnetGenesisBlockParent).value()};
    v.resize(0);
    v.mainnet_genesis = cids == mainnet;
    if (!v.mainnet_genesis) {
      for (const auto &_cid : cids) {
        if (auto cid{asBlake(_cid)}) {
          v.push_back(*cid);
        } else {
          outcome::raise(JsonError::kWrongType);
        }
      }
    }
  }
}  // namespace fc

namespace fc::primitives {
  using codec::json::encode;
  using codec::json::innerDecode;

  JSON_ENCODE(RleBitset) {
    return encode(codec::rle::toRuns(v), allocator);
  }

  JSON_DECODE(RleBitset) {
    v = codec::rle::fromRuns(innerDecode<codec::rle::Runs64>(j));
  }

  JSON_ENCODE(TaskType) {
    return encode(std::string_view(v), allocator);
  }

  JSON_DECODE(TaskType) {
    v = TaskType(AsString(j));
  }

  namespace address {
    using codec::json::encode;
    JSON_ENCODE(Address) {
      return encode(encodeToString(v), allocator);
    }

    JSON_DECODE(Address) {
      OUTCOME_EXCEPT(decoded, decodeFromString(AsString(j)));
      v = std::move(decoded);
    }
  }  // namespace address

  namespace sector_file {
    using codec::json::decodeEnum;
    using codec::json::encode;

    JSON_ENCODE(SectorFileType) {
      return encode(common::to_int(v), allocator);
    }

    JSON_DECODE(SectorFileType) {
      decodeEnum(v, j);
    }
  }  // namespace sector_file

  namespace sector {
    using codec::json::decodeEnum;
    using codec::json::encode;

    JSON_ENCODE(RegisteredSealProof) {
      return encode(common::to_int(v), allocator);
    }

    JSON_DECODE(RegisteredSealProof) {
      decodeEnum(v, j);
    }

    JSON_ENCODE(RegisteredPoStProof) {
      return encode(common::to_int(v), allocator);
    }

    JSON_DECODE(RegisteredPoStProof) {
      decodeEnum(v, j);
    }

    JSON_ENCODE(PoStProof) {
      Value j{rapidjson::kObjectType};
      Set(j, "PoStProof", v.registered_proof, allocator);
      Set(j, "ProofBytes", gsl::make_span(v.proof), allocator);
      return j;
    }

    JSON_DECODE(PoStProof) {
      Get(j, "PoStProof", v.registered_proof);
      Get(j, "ProofBytes", v.proof);
    }

    JSON_ENCODE(SectorId) {
      Value j{rapidjson::kObjectType};
      Set(j, "Miner", v.miner, allocator);
      Set(j, "Number", v.sector, allocator);
      return j;
    }

    JSON_DECODE(SectorId) {
      Get(j, "Miner", v.miner);
      Get(j, "Number", v.sector);
    }

    JSON_ENCODE(ExtendedSectorInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "SealProof", v.registered_proof, allocator);
      Set(j, "SectorKey", v.sector_key, allocator);
      Set(j, "SectorNumber", v.sector, allocator);
      Set(j, "SealedCID", v.sealed_cid, allocator);
      return j;
    }

    JSON_DECODE(ExtendedSectorInfo) {
      Get(j, "SealProof", v.registered_proof);
      Get(j, "SectorKey", v.sector_key);
      Get(j, "SectorNumber", v.sector);
      Get(j, "SealedCID", v.sealed_cid);
    }

    JSON_ENCODE(SectorRef) {
      Value j{rapidjson::kObjectType};
      Set(j, "ID", v.id, allocator);
      Set(j, "ProofType", v.proof_type, allocator);
      return j;
    }

    JSON_DECODE(SectorRef) {
      Get(j, "ID", v.id);
      Get(j, "ProofType", v.proof_type);
    }
  }  // namespace sector

  namespace piece {
    using codec::json::encode;
    using codec::json::innerDecode;
    using fc::codec::json::Get;
    using fc::codec::json::Set;
    using fc::codec::json::Value;

    JSON_ENCODE(PaddedPieceSize) {
      return encode(static_cast<uint64_t>(v), allocator);
    }

    JSON_DECODE(PaddedPieceSize) {
      v = innerDecode<uint64_t>(j);
    }

    JSON_ENCODE(UnpaddedPieceSize) {
      return encode(static_cast<uint64_t>(v), allocator);
    }

    JSON_DECODE(UnpaddedPieceSize) {
      v = innerDecode<uint64_t>(j);
    }

    JSON_ENCODE(PieceInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "Size", v.size, allocator);
      Set(j, "PieceCID", v.cid, allocator);
      return j;
    }

    JSON_DECODE(PieceInfo) {
      Get(j, "Size", v.size);
      Get(j, "PieceCID", v.cid);
    }

    JSON_ENCODE(MetaPieceData) {
      Value j{rapidjson::kObjectType};
      Set(j, "Type", v.type.toString(), allocator);
      Set(j, "Info", v.info, allocator);
      return j;
    }

    JSON_DECODE(MetaPieceData) {
      std::string type;
      Get(j, "Type", type);
      v.type = ReaderType::fromString(type);
      Get(j, "Info", v.info);
    }

  }  // namespace piece

  namespace block {
    using codec::json::Get;
    using codec::json::Set;
    using codec::json::Value;

    JSON_ENCODE(Ticket) {
      Value j{rapidjson::kObjectType};
      Set(j, "VRFProof", gsl::make_span(v.bytes), allocator);
      return j;
    }

    JSON_DECODE(Ticket) {
      Get(j, "VRFProof", v.bytes);
    }

    JSON_ENCODE(ElectionProof) {
      Value j{rapidjson::kObjectType};
      Set(j, "WinCount", v.win_count, allocator);
      Set(j, "VRFProof", v.vrf_proof, allocator);
      return j;
    }

    JSON_DECODE(ElectionProof) {
      Get(j, "WinCount", v.win_count);
      Get(j, "VRFProof", v.vrf_proof);
    }

    JSON_ENCODE(BlockHeader) {
      Value j{rapidjson::kObjectType};
      Set(j, "Miner", v.miner, allocator);
      Set(j, "Ticket", v.ticket, allocator);
      Set(j, "ElectionProof", v.election_proof, allocator);
      Set(j, "BeaconEntries", v.beacon_entries, allocator);
      Set(j, "WinPoStProof", v.win_post_proof, allocator);
      Set(j, "Parents", v.parents, allocator);
      Set(j, "ParentWeight", v.parent_weight, allocator);
      Set(j, "Height", v.height, allocator);
      Set(j, "ParentStateRoot", v.parent_state_root, allocator);
      Set(j, "ParentMessageReceipts", v.parent_message_receipts, allocator);
      Set(j, "Messages", v.messages, allocator);
      Set(j, "BLSAggregate", v.bls_aggregate, allocator);
      Set(j, "Timestamp", v.timestamp, allocator);
      Set(j, "BlockSig", v.block_sig, allocator);
      Set(j, "ForkSignaling", v.fork_signaling, allocator);
      Set(j, "ParentBaseFee", v.parent_base_fee, allocator);
      return j;
    }

    JSON_DECODE(BlockHeader) {
      Get(j, "Miner", v.miner);
      Get(j, "Ticket", v.ticket);
      Get(j, "ElectionProof", v.election_proof);
      Get(j, "BeaconEntries", v.beacon_entries);
      Get(j, "WinPoStProof", v.win_post_proof);
      Get(j, "Parents", v.parents);
      Get(j, "ParentWeight", v.parent_weight);
      Get(j, "Height", v.height);
      Get(j, "ParentStateRoot", v.parent_state_root);
      Get(j, "ParentMessageReceipts", v.parent_message_receipts);
      Get(j, "Messages", v.messages);
      Get(j, "BLSAggregate", v.bls_aggregate);
      Get(j, "Timestamp", v.timestamp);
      Get(j, "BlockSig", v.block_sig);
      Get(j, "ForkSignaling", v.fork_signaling);
      Get(j, "ParentBaseFee", v.parent_base_fee);
    }

    JSON_ENCODE(BlockTemplate) {
      Value j{rapidjson::kObjectType};
      Set(j, "Miner", v.miner, allocator);
      Set(j, "Parents", v.parents, allocator);
      Set(j, "Ticket", v.ticket, allocator);
      Set(j, "Eproof", v.election_proof, allocator);
      Set(j, "BeaconValues", v.beacon_entries, allocator);
      Set(j, "Messages", v.messages, allocator);
      Set(j, "Epoch", v.height, allocator);
      Set(j, "Timestamp", v.timestamp, allocator);
      Set(j, "WinningPoStProof", v.win_post_proof, allocator);
      return j;
    }

    JSON_DECODE(BlockTemplate) {
      Get(j, "Miner", v.miner);
      Get(j, "Parents", v.parents);
      Get(j, "Ticket", v.ticket);
      Get(j, "Eproof", v.election_proof);
      Get(j, "BeaconValues", v.beacon_entries);
      Get(j, "Messages", v.messages);
      Get(j, "Epoch", v.height);
      Get(j, "Timestamp", v.timestamp);
      Get(j, "WinningPoStProof", v.win_post_proof);
    }

    JSON_ENCODE(BlockWithCids) {
      Value j{rapidjson::kObjectType};
      Set(j, "Header", v.header, allocator);
      Set(j, "BlsMessages", v.bls_messages, allocator);
      Set(j, "SecpkMessages", v.secp_messages, allocator);
      return j;
    }

    JSON_DECODE(BlockWithCids) {
      Get(j, "Header", v.header);
      Get(j, "BlsMessages", v.bls_messages);
      Get(j, "SecpkMessages", v.secp_messages);
    }
  }  // namespace block

  namespace tipset {
    using codec::json::AsString;
    using codec::json::encode;
    using codec::json::Get;
    using codec::json::innerDecode;
    using codec::json::JsonError;
    using codec::json::Set;
    using codec::json::Value;

    JSON_ENCODE(TipsetKey) {
      return encode(v.cids(), allocator);
    }

    JSON_DECODE(TipsetKey) {
      v = innerDecode<std::vector<CbCid>>(j);
    }

    JSON_ENCODE(TipsetCPtr) {
      Value j{rapidjson::kObjectType};
      Set(j, "Cids", v->key.cids(), allocator);
      Set(j, "Blocks", v->blks, allocator);
      Set(j, "Height", v->height(), allocator);
      return j;
    }

    JSON_DECODE(TipsetCPtr) {
      std::vector<primitives::block::BlockHeader> blks;
      Get(j, "Blocks", blks);

      // TODO (a.gorbachev) decode and verify excessive values

      OUTCOME_EXCEPT(tipset,
                     primitives::tipset::Tipset::create(std::move(blks)));
      v = std::move(tipset);
    }

    JSON_ENCODE(HeadChange) {
      const char *type{};
      if (v.type == HeadChangeType::CURRENT) {
        type = "current";
      } else if (v.type == HeadChangeType::REVERT) {
        type = "revert";
      } else if (v.type == HeadChangeType::APPLY) {
        type = "apply";
      }
      Value j{rapidjson::kObjectType};
      Set(j, "Type", type, allocator);
      Set(j, "Val", v.value, allocator);
      return j;
    }

    JSON_DECODE(HeadChange) {
      auto type = AsString(Get(j, "Type"));
      if (type == "current") {
        v.type = HeadChangeType::CURRENT;
      } else if (type == "revert") {
        v.type = HeadChangeType::REVERT;
      } else if (type == "apply") {
        v.type = HeadChangeType::APPLY;
      } else {
        outcome::raise(JsonError::kWrongEnum);
      }
      Get(j, "Val", v.value);
    }
  }  // namespace tipset

}  // namespace fc::primitives

namespace fc::proofs {
  JSON_ENCODE(SealedAndUnsealedCID) {
    Value j{rapidjson::kObjectType};
    Set(j, "Sealed", v.sealed_cid, allocator);
    Set(j, "Unsealed", v.unsealed_cid, allocator);
    return j;
  }

  JSON_DECODE(SealedAndUnsealedCID) {
    Get(j, "Sealed", v.sealed_cid);
    Get(j, "Unsealed", v.unsealed_cid);
  }
}  // namespace fc::proofs

namespace fc::crypto {

  namespace randomness {
    using codec::json::decodeEnum;
    using codec::json::encode;

    JSON_ENCODE(DomainSeparationTag) {
      return encode(common::to_int(v), allocator);
    }

    JSON_DECODE(DomainSeparationTag) {
      decodeEnum(v, j);
    }
  }  // namespace randomness

  namespace signature {
    using codec::json::Get;
    using codec::json::innerDecode;
    using codec::json::JsonError;
    using codec::json::Set;
    using codec::json::Value;

    JSON_ENCODE(Signature) {
      uint64_t type = Type::kUndefined;
      gsl::span<const uint8_t> data;
      visit_in_place(
          v,
          [&](const BlsSignature &bls) {
            type = Type::kBls;
            data = gsl::make_span(bls);
          },
          [&](const Secp256k1Signature &secp) {
            type = Type::kSecp256k1;
            data = gsl::make_span(secp);
          });
      Value j{rapidjson::kObjectType};
      Set(j, "Type", type, allocator);
      Set(j, "Data", data, allocator);
      return j;
    }

    JSON_DECODE(Signature) {
      uint64_t type = Type::kUndefined;
      Get(j, "Type", type);
      const auto &data = Get(j, "Data");
      if (type == Type::kBls) {
        v = innerDecode<BlsSignature>(data);
      } else if (type == Type::kSecp256k1) {
        v = innerDecode<Secp256k1Signature>(data);
      } else {
        outcome::raise(JsonError::kWrongEnum);
      }
    }
  }  // namespace signature
}  // namespace fc::crypto

namespace fc::sector_storage {
  using codec::json::decodeEnum;
  using codec::json::encode;
  using common::fromString;
  using common::toString;

  namespace stores {
    using codec::json::encode;

    JSON_ENCODE(AcquireMode) {
      return encode(toString(v), allocator);
    }

    JSON_DECODE(AcquireMode) {
      v = fromString<AcquireMode>(AsString(j)).value();
    }

    JSON_ENCODE(PathType) {
      return encode(toString(v).value(), allocator);
    }

    JSON_DECODE(PathType) {
      v = fromString<PathType>(AsString(j)).value();
    }

    JSON_ENCODE(HealthReport) {
      Value j{rapidjson::kObjectType};
      Set(j, "Stat", v.stat, allocator);
      return j;
    }

    JSON_DECODE(HealthReport) {
      Get(j, "Stat", v.stat);
    }

    JSON_ENCODE(StorageInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "ID", v.id, allocator);
      Set(j, "URLs", v.urls, allocator);
      Set(j, "Weight", v.weight, allocator);
      Set(j, "CanSeal", v.can_seal, allocator);
      Set(j, "CanStore", v.can_store, allocator);
      return j;
    }

    JSON_DECODE(StorageInfo) {
      Get(j, "ID", v.id);
      Get(j, "URLs", v.urls);
      Get(j, "Weight", v.weight);
      Get(j, "CanSeal", v.can_seal);
      Get(j, "CanStore", v.can_store);
    }

    JSON_ENCODE(SectorStorageInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "ID", v.id, allocator);
      Set(j, "URLs", v.urls, allocator);
      Set(j, "Weight", v.weight, allocator);
      Set(j, "CanSeal", v.can_seal, allocator);
      Set(j, "CanStore", v.can_store, allocator);
      Set(j, "Primary", v.is_primary, allocator);
      return j;
    }

    JSON_DECODE(SectorStorageInfo) {
      Get(j, "ID", v.id);
      Get(j, "URLs", v.urls);
      Get(j, "Weight", v.weight);
      Get(j, "CanSeal", v.can_seal);
      Get(j, "CanStore", v.can_store);
      Get(j, "Primary", v.is_primary);
    }
  }  // namespace stores

  JSON_ENCODE(CallErrorCode) {
    return encode(common::to_int(v), allocator);
  }

  JSON_DECODE(CallErrorCode) {
    decodeEnum(v, j);
  }

  JSON_ENCODE(CallError) {
    Value j{rapidjson::kObjectType};
    Set(j, "Code", v.code, allocator);
    Set(j, "Message", v.message, allocator);
    return j;
  }

  JSON_DECODE(CallError) {
    Get(j, "Code", v.code);
    Get(j, "Message", v.message);
  }

  JSON_ENCODE(CallId) {
    Value j{rapidjson::kObjectType};
    Set(j, "Sector", v.sector, allocator);
    Set(j, "ID", v.id, allocator);
    return j;
  }

  JSON_DECODE(CallId) {
    Get(j, "Sector", v.sector);
    Get(j, "ID", v.id);
  }

  JSON_ENCODE(Range) {
    Value j{rapidjson::kObjectType};
    Set(j, "Offset", v.offset, allocator);
    Set(j, "Size", v.size, allocator);
    return j;
  }

  JSON_DECODE(Range) {
    Get(j, "Offset", v.offset);
    Get(j, "Size", v.size);
  }
}  // namespace fc::sector_storage

namespace fc::adt {
  using codec::json::encode;
  using codec::json::innerDecode;

  template <typename T>
  JSON_ENCODE(adt::Array<T>) {
    return encode(v.amt.cid());
  }

  template <typename T>
  JSON_DECODE(adt::Array<T>) {
    v.amt = {nullptr, innerDecode<CID>(j)};
  }
}  // namespace fc::adt

namespace fc::storage::mpool {
  using fc::codec::json::decodeEnum;
  using fc::codec::json::Get;
  using fc::codec::json::Set;
  using fc::codec::json::Value;

  JSON_ENCODE(MpoolUpdate) {
    Value j{rapidjson::kObjectType};
    Set(j, "Type", common::to_int(v.type), allocator);
    Set(j, "Message", v.message, allocator);
    return j;
  }

  JSON_DECODE(MpoolUpdate) {
    decodeEnum(v.type, Get(j, "Type"));
    Get(j, "Message", v.message);
  }
}  // namespace fc::storage::mpool

namespace fc::drand {
  using codec::json::Get;
  using codec::json::Set;
  using codec::json::Value;

  JSON_ENCODE(BeaconEntry) {
    Value j{rapidjson::kObjectType};
    Set(j, "Round", v.round, allocator);
    Set(j, "Data", v.data, allocator);
    return j;
  }

  JSON_DECODE(BeaconEntry) {
    Get(j, "Round", v.round);
    Get(j, "Data", v.data);
  }
}  // namespace fc::drand

namespace fc::mining {
  using codec::json::decodeEnum;
  using codec::json::encode;

  namespace types {
    using codec::json::Get;
    using codec::json::Set;
    using codec::json::Value;

    JSON_ENCODE(DealSchedule) {
      Value j{rapidjson::kObjectType};
      Set(j, "StartEpoch", v.start_epoch, allocator);
      Set(j, "EndEpoch", v.end_epoch, allocator);
      return j;
    }

    JSON_DECODE(DealSchedule) {
      Get(j, "StartEpoch", v.end_epoch);
      Get(j, "EndEpoch", v.end_epoch);
    }

    JSON_ENCODE(DealInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "PublishCid", v.publish_cid, allocator);
      Set(j, "DealID", v.deal_id, allocator);
      Set(j, "DealProposal", v.deal_proposal, allocator);
      Set(j, "DealSchedule", v.deal_schedule, allocator);
      Set(j, "KeepUnsealed", v.is_keep_unsealed, allocator);
      return j;
    }

    JSON_DECODE(DealInfo) {
      Get(j, "PublishCid", v.publish_cid);
      Get(j, "DealID", v.deal_id);
      Get(j, "DealProposal", v.deal_proposal);
      Get(j, "DealSchedule", v.deal_schedule);
      Get(j, "KeepUnsealed", v.is_keep_unsealed);
    }

    JSON_ENCODE(Piece) {
      Value j{rapidjson::kObjectType};
      Set(j, "Piece", v.piece, allocator);
      Set(j, "DealInfo", v.deal_info, allocator);
      return j;
    }

    JSON_DECODE(Piece) {
      Get(j, "Piece", v.piece);
      Get(j, "DealInfo", v.deal_info);
    }
  }  // namespace types

  JSON_ENCODE(SealingState) {
    return encode(common::to_int(v), allocator);
  }

  JSON_DECODE(SealingState) {
    decodeEnum(v, j);
  }
}  // namespace fc::mining

namespace fc::miner::types {
  using codec::json::Get;
  using codec::json::Set;
  using codec::json::Value;

  JSON_ENCODE(PreSealSector) {
    Value j{rapidjson::kObjectType};
    Set(j, "CommR", v.comm_r, allocator);
    Set(j, "CommD", v.comm_d, allocator);
    Set(j, "SectorID", v.sector_id, allocator);
    Set(j, "Deal", v.deal, allocator);
    Set(j, "ProofType", v.proof_type, allocator);
    return j;
  }

  JSON_DECODE(PreSealSector) {
    Get(j, "CommR", v.comm_r);
    Get(j, "CommD", v.comm_d);
    Get(j, "SectorID", v.sector_id);
    Get(j, "Deal", v.deal);
    Get(j, "ProofType", v.proof_type);
  }

  JSON_ENCODE(Miner) {
    Value j{rapidjson::kObjectType};
    Set(j, "ID", v.id, allocator);
    Set(j, "Owner", v.owner, allocator);
    Set(j, "Worker", v.worker, allocator);
    Set(j, "PeerId", v.peer_id, allocator);
    Set(j, "MarketBalance", v.market_balance, allocator);
    Set(j, "PowerBalance", v.power_balance, allocator);
    Set(j, "SectorSize", v.sector_size, allocator);
    Set(j, "Sectors", v.sectors, allocator);
    return j;
  }

  JSON_DECODE(Miner) {
    Get(j, "ID", v.id);
    Get(j, "Owner", v.owner);
    Get(j, "Worker", v.worker);
    Get(j, "PeerId", v.peer_id);
    Get(j, "MarketBalance", v.market_balance);
    Get(j, "PowerBalance", v.power_balance);
    Get(j, "SectorSize", v.sector_size);
    Get(j, "Sectors", v.sectors);
  }
}  // namespace fc::miner::types

namespace fc::vm::actor::builtin::types {
  namespace miner {
    using codec::json::Get;
    using codec::json::Set;
    using codec::json::Value;

    JSON_ENCODE(SectorPreCommitOnChainInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "Info", v.info, allocator);
      Set(j, "PreCommitDeposit", v.precommit_deposit, allocator);
      Set(j, "PreCommitEpoch", v.precommit_epoch, allocator);
      Set(j, "DealWeight", v.deal_weight, allocator);
      Set(j, "VerifiedDealWeight", v.verified_deal_weight, allocator);
      return j;
    }

    JSON_DECODE(SectorPreCommitOnChainInfo) {
      Get(j, "Info", v.info);
      Get(j, "PreCommitDeposit", v.precommit_deposit);
      Get(j, "PreCommitEpoch", v.precommit_epoch);
      Get(j, "DealWeight", v.deal_weight);
      Get(j, "VerifiedDealWeight", v.verified_deal_weight);
    }

    JSON_ENCODE(DeadlineInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "CurrentEpoch", v.current_epoch, allocator);
      Set(j, "PeriodStart", v.period_start, allocator);
      Set(j, "Index", v.index, allocator);
      Set(j, "Open", v.open, allocator);
      Set(j, "Close", v.close, allocator);
      Set(j, "Challenge", v.challenge, allocator);
      Set(j, "FaultCutoff", v.fault_cutoff, allocator);
      Set(j, "WPoStPeriodDeadlines", v.wpost_period_deadlines, allocator);
      Set(j, "WPoStProvingPeriod", v.wpost_proving_period, allocator);
      Set(j, "WPoStChallengeWindow", v.wpost_challenge_window, allocator);
      Set(j, "WPoStChallengeLookback", v.wpost_challenge_lookback, allocator);
      Set(j, "FaultDeclarationCutoff", v.fault_declaration_cutoff, allocator);
      return j;
    }

    JSON_DECODE(DeadlineInfo) {
      Get(j, "CurrentEpoch", v.current_epoch);
      Get(j, "PeriodStart", v.period_start);
      Get(j, "Index", v.index);
      Get(j, "Open", v.open);
      Get(j, "Close", v.close);
      Get(j, "Challenge", v.challenge);
      Get(j, "FaultCutoff", v.fault_cutoff);
      Get(j, "WPoStPeriodDeadlines", v.wpost_period_deadlines);
      Get(j, "WPoStProvingPeriod", v.wpost_proving_period);
      Get(j, "WPoStChallengeWindow", v.wpost_challenge_window);
      Get(j, "WPoStChallengeLookback", v.wpost_challenge_lookback);
      Get(j, "FaultDeclarationCutoff", v.fault_declaration_cutoff);
    }

    JSON_ENCODE(SectorOnChainInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "SectorNumber", v.sector, allocator);
      Set(j, "SealProof", v.seal_proof, allocator);
      Set(j, "SealedCID", v.sealed_cid, allocator);
      Set(j, "DealIDs", v.deals, allocator);
      Set(j, "Activation", v.activation_epoch, allocator);
      Set(j, "Expiration", v.expiration, allocator);
      Set(j, "DealWeight", v.deal_weight, allocator);
      Set(j, "VerifiedDealWeight", v.verified_deal_weight, allocator);
      Set(j, "InitialPledge", v.init_pledge, allocator);
      Set(j, "ExpectedDayReward", v.expected_day_reward, allocator);
      Set(j, "ExpectedStoragePledge", v.expected_storage_pledge, allocator);
      return j;
    }

    JSON_DECODE(SectorOnChainInfo) {
      Get(j, "SectorNumber", v.sector);
      Get(j, "SealProof", v.seal_proof);
      Get(j, "SealedCID", v.sealed_cid);
      Get(j, "DealIDs", v.deals);
      Get(j, "Activation", v.activation_epoch);
      Get(j, "Expiration", v.expiration);
      Get(j, "DealWeight", v.deal_weight);
      Get(j, "VerifiedDealWeight", v.verified_deal_weight);
      Get(j, "InitialPledge", v.init_pledge);
      Get(j, "ExpectedDayReward", v.expected_day_reward);
      Get(j, "ExpectedStoragePledge", v.expected_storage_pledge);
    }

    JSON_ENCODE(SectorPreCommitInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "SealProof", v.registered_proof, allocator);
      Set(j, "SectorNumber", v.sector, allocator);
      Set(j, "SealedCID", v.sealed_cid, allocator);
      Set(j, "SealRandEpoch", v.seal_epoch, allocator);
      Set(j, "DealIDs", v.deal_ids, allocator);
      Set(j, "Expiration", v.expiration, allocator);
      return j;
    }

    JSON_DECODE(SectorPreCommitInfo) {
      Get(j, "SealProof", v.registered_proof);
      Get(j, "SectorNumber", v.sector);
      Get(j, "SealedCID", v.sealed_cid);
      Get(j, "SealRandEpoch", v.seal_epoch);
      Get(j, "DealIDs", v.deal_ids);
      Get(j, "Expiration", v.expiration);
    }
  }  // namespace miner

  namespace market {
    using codec::json::Get;
    using codec::json::Set;
    using codec::json::Value;

    JSON_ENCODE(DealState) {
      Value j{rapidjson::kObjectType};
      Set(j, "SectorStartEpoch", v.sector_start_epoch, allocator);
      Set(j, "LastUpdatedEpoch", v.last_updated_epoch, allocator);
      Set(j, "SlashEpoch", v.slash_epoch, allocator);
      return j;
    }

    JSON_DECODE(DealState) {
      Get(j, "SectorStartEpoch", v.sector_start_epoch);
      Get(j, "LastUpdatedEpoch", v.last_updated_epoch);
      Get(j, "SlashEpoch", v.slash_epoch);
    }

    JSON_ENCODE(Universal<DealProposal>) {
      Value j{rapidjson::kObjectType};
      Set(j, "PieceCID", v->piece_cid, allocator);
      Set(j, "PieceSize", v->piece_size, allocator);
      Set(j, "VerifiedDeal", v->verified, allocator);
      Set(j, "Client", v->client, allocator);
      Set(j, "Provider", v->provider, allocator);
      Set(j, "StartEpoch", v->start_epoch, allocator);
      Set(j, "EndEpoch", v->end_epoch, allocator);
      Set(j, "StoragePricePerEpoch", v->storage_price_per_epoch, allocator);
      Set(j, "ProviderCollateral", v->provider_collateral, allocator);
      Set(j, "ClientCollateral", v->client_collateral, allocator);
      return j;
    }

    JSON_DECODE(Universal<DealProposal>) {
      // TODO (a.chernyshov) use V8 actor asap
      vm::actor::builtin::v0::market::DealProposal deal_proposal;
      Get(j, "PieceCID", deal_proposal.piece_cid);
      Get(j, "PieceSize", deal_proposal.piece_size);
      Get(j, "VerifiedDeal", deal_proposal.verified);
      Get(j, "Client", deal_proposal.client);
      Get(j, "Provider", deal_proposal.provider);
      Get(j, "StartEpoch", deal_proposal.start_epoch);
      Get(j, "EndEpoch", deal_proposal.end_epoch);
      Get(j, "StoragePricePerEpoch", deal_proposal.storage_price_per_epoch);
      Get(j, "ProviderCollateral", deal_proposal.provider_collateral);
      Get(j, "ClientCollateral", deal_proposal.client_collateral);

      v = vm::actor::builtin::types::Universal<
          vm::actor::builtin::types::market::DealProposal>(
          vm::actor::ActorVersion::kVersion0,
          std::make_shared<vm::actor::builtin::v0::market::DealProposal>(
              deal_proposal));
    }

    JSON_ENCODE(DealProposal) {
      Value j{rapidjson::kObjectType};
      Set(j, "PieceCID", v.piece_cid, allocator);
      Set(j, "PieceSize", v.piece_size, allocator);
      Set(j, "VerifiedDeal", v.verified, allocator);
      Set(j, "Client", v.client, allocator);
      Set(j, "Provider", v.provider, allocator);
      Set(j, "StartEpoch", v.start_epoch, allocator);
      Set(j, "EndEpoch", v.end_epoch, allocator);
      Set(j, "StoragePricePerEpoch", v.storage_price_per_epoch, allocator);
      Set(j, "ProviderCollateral", v.provider_collateral, allocator);
      Set(j, "ClientCollateral", v.client_collateral, allocator);
      return j;
    }

    JSON_DECODE(DealProposal) {
      Get(j, "PieceCID", v.piece_cid);
      Get(j, "PieceSize", v.piece_size);
      Get(j, "VerifiedDeal", v.verified);
      Get(j, "Client", v.client);
      Get(j, "Provider", v.provider);
      Get(j, "StartEpoch", v.start_epoch);
      Get(j, "EndEpoch", v.end_epoch);
      Get(j, "StoragePricePerEpoch", v.storage_price_per_epoch);
      Get(j, "ProviderCollateral", v.provider_collateral);
      Get(j, "ClientCollateral", v.client_collateral);
    }
  }  // namespace market

  namespace payment_channel {
    using codec::json::Get;
    using codec::json::Set;
    using codec::json::Value;
    using crypto::signature::Signature;

    JSON_ENCODE(SignedVoucher) {
      Value j{rapidjson::kObjectType};
      Set(j, "ChannelAddr", v.channel, allocator);
      Set(j, "TimeLockMin", v.time_lock_min, allocator);
      Set(j, "TimeLockMax", v.time_lock_max, allocator);
      Set(j, "SecretPreimage", gsl::make_span(v.secret_preimage), allocator);
      Set(j, "Extra", v.extra, allocator);
      Set(j, "Lane", v.lane, allocator);
      Set(j, "Nonce", v.nonce, allocator);
      Set(j, "Amount", v.amount, allocator);
      Set(j, "MinSettleHeight", v.min_close_height, allocator);
      Set(j, "Merges", v.merges, allocator);
      const auto sig{v.signature_bytes.map(
          [](auto &bytes) { return Signature::fromBytes(bytes).value(); })};
      Set(j, "Signature", sig, allocator);
      return j;
    }

    JSON_DECODE(SignedVoucher) {
      Get(j, "ChannelAddr", v.channel);
      Get(j, "TimeLockMin", v.time_lock_min);
      Get(j, "TimeLockMax", v.time_lock_max);
      Get(j, "SecretPreimage", v.secret_preimage);
      Get(j, "Extra", v.extra);
      Get(j, "Lane", v.lane);
      Get(j, "Nonce", v.nonce);
      Get(j, "Amount", v.amount);
      Get(j, "MinSettleHeight", v.min_close_height);
      Get(j, "Merges", v.merges);
      boost::optional<Signature> sig;
      Get(j, "Signature", sig);
      v.signature_bytes = sig.map([](auto &sig) { return sig.toBytes(); });
    }

    JSON_ENCODE(ModularVerificationParameter) {
      Value j{rapidjson::kObjectType};
      Set(j, "Actor", v.actor, allocator);
      Set(j, "Method", v.method, allocator);
      Set(j, "Params", gsl::make_span(v.params), allocator);
      return j;
    }

    JSON_DECODE(ModularVerificationParameter) {
      Get(j, "Actor", v.actor);
      Get(j, "Method", v.method);
      Get(j, "Params", v.params);
    }

    JSON_ENCODE(Merge) {
      Value j{rapidjson::kObjectType};
      Set(j, "Lane", v.lane, allocator);
      Set(j, "Nonce", v.nonce, allocator);
      return j;
    }

    JSON_DECODE(Merge) {
      Get(j, "Lane", v.lane);
      Get(j, "Nonce", v.nonce);
    }
  }  // namespace payment_channel

  namespace storage_power {
    using codec::json::Get;
    using codec::json::Set;
    using codec::json::Value;

    JSON_ENCODE(Claim) {
      Value j{rapidjson::kObjectType};
      Set(j, "RawBytePower", v.raw_power, allocator);
      Set(j, "QualityAdjPower", v.qa_power, allocator);
      return j;
    }

    JSON_DECODE(Claim) {
      Get(j, "RawBytePower", v.raw_power);
      Get(j, "QualityAdjPower", v.qa_power);
    }
  }  // namespace storage_power
}  // namespace fc::vm::actor::builtin::types

namespace fc::vm {
  namespace version {
    using codec::json::decodeEnum;
    using codec::json::encode;

    JSON_ENCODE(NetworkVersion) {
      return encode(common::to_int(v), allocator);
    }

    JSON_DECODE(NetworkVersion) {
      decodeEnum(v, j);
    }
  }  // namespace version
  namespace actor {

    JSON_ENCODE(Actor) {
      Value j{rapidjson::kObjectType};
      Set(j, "Code", v.code, allocator);
      Set(j, "Head", v.head, allocator);
      Set(j, "Nonce", v.nonce, allocator);
      Set(j, "Balance", v.balance, allocator);
      return j;
    }

    JSON_DECODE(Actor) {
      Get(j, "Code", v.code);
      Get(j, "Head", v.head);
      Get(j, "Nonce", v.nonce);
      Get(j, "Balance", v.balance);
    }
  }  // namespace actor
  namespace runtime {
    using codec::json::decodeEnum;
    using codec::json::Get;
    using codec::json::Set;
    using codec::json::Value;

    JSON_ENCODE(MessageReceipt) {
      Value j{rapidjson::kObjectType};
      Set(j, "ExitCode", common::to_int(v.exit_code), allocator);
      Set(j, "Return", gsl::make_span(v.return_value), allocator);
      Set(j, "GasUsed", v.gas_used, allocator);
      return j;
    }

    JSON_DECODE(MessageReceipt) {
      decodeEnum(v.exit_code, Get(j, "ExitCode"));
      Get(j, "Return", v.return_value);
      Get(j, "GasUsed", v.gas_used);
    }

    JSON_ENCODE(ExecutionResult) {
      Value j{rapidjson::kObjectType};
      Set(j, "Msg", v.message, allocator);
      Set(j, "MsgRct", v.receipt, allocator);
      Set(j, "Error", v.error, allocator);
      return j;
    }

    JSON_DECODE(ExecutionResult) {
      Get(j, "Msg", v.message);
      Get(j, "MsgRct", v.receipt);
      v.error = AsString(Get(j, "Error"));
    }
  }  // namespace runtime
  namespace message {
    using codec::json::Get;
    using codec::json::Set;
    using codec::json::Value;
    using primitives::cid::getCidOfCbor;

    JSON_ENCODE(UnsignedMessage) {
      Value j{rapidjson::kObjectType};
      Set(j, "Version", v.version, allocator);
      Set(j, "To", v.to, allocator);
      Set(j, "From", v.from, allocator);
      Set(j, "Nonce", v.nonce, allocator);
      Set(j, "Value", v.value, allocator);
      Set(j, "GasLimit", v.gas_limit, allocator);
      Set(j, "GasFeeCap", v.gas_fee_cap, allocator);
      Set(j, "GasPremium", v.gas_premium, allocator);
      Set(j, "Method", v.method, allocator);
      Set(j, "Params", gsl::make_span(v.params), allocator);
      return j;
    }

    JSON_DECODE(UnsignedMessage) {
      Get(j, "Version", v.version);
      Get(j, "To", v.to);
      Get(j, "From", v.from);
      Get(j, "Nonce", v.nonce);
      Get(j, "Value", v.value);
      Get(j, "GasLimit", v.gas_limit);
      Get(j, "GasFeeCap", v.gas_fee_cap);
      Get(j, "GasPremium", v.gas_premium);
      Get(j, "Method", v.method);
      Get(j, "Params", v.params);
    }

    JSON_ENCODE(SignedMessage) {
      Value j{rapidjson::kObjectType};
      Set(j, "Message", v.message, allocator);
      Set(j, "Signature", v.signature, allocator);
      OUTCOME_EXCEPT(
          cid, v.signature.isBls() ? getCidOfCbor(v.message) : getCidOfCbor(v));
      Set(j, "CID", cid, allocator);
      return j;
    }

    JSON_DECODE(SignedMessage) {
      Get(j, "Message", v.message);
      Get(j, "Signature", v.signature);
    }
  }  // namespace message
}  // namespace fc::vm

namespace fc::markets::retrieval {
  using codec::json::Get;
  using codec::json::Set;
  using codec::json::Value;

  namespace client {
    using codec::json::Get;
    using codec::json::Set;
    using codec::json::Value;

    JSON_ENCODE(RetrievalDeal) {
      Value j{rapidjson::kObjectType};
      Set(j, "Proposal", v.proposal, allocator);
      Set(j, "Accepted", v.accepted, allocator);
      Set(j, "AllBlocks", v.all_blocks, allocator);
      Set(j, "Client", v.client_wallet, allocator);
      Set(j, "Provider", v.miner_wallet, allocator);
      Set(j, "TotalFunds", v.total_funds, allocator);
      return j;
    }

    JSON_DECODE(RetrievalDeal) {
      Get(j, "Proposal", v.proposal);
      Get(j, "Accepted", v.accepted);
      Get(j, "AllBlocks", v.all_blocks);
      Get(j, "Client", v.client_wallet);
      Get(j, "Provider", v.miner_wallet);
      Get(j, "TotalFunds", v.total_funds);
    }
  }  // namespace client

  JSON_ENCODE(RetrievalAsk) {
    Value j{rapidjson::kObjectType};
    Set(j, "PricePerByte", v.price_per_byte, allocator);
    Set(j, "UnsealPrice", v.unseal_price, allocator);
    Set(j, "PaymentInterval", v.payment_interval, allocator);
    Set(j, "PaymentIntervalIncrease", v.interval_increase, allocator);
    return j;
  }

  JSON_DECODE(RetrievalAsk) {
    Get(j, "PricePerByte", v.price_per_byte);
    Get(j, "UnsealPrice", v.unseal_price);
    Get(j, "PaymentInterval", v.payment_interval);
    Get(j, "PaymentIntervalIncrease", v.interval_increase);
  }

  JSON_ENCODE(DealProposalV1_0_0) {
    Value j{rapidjson::kObjectType};
    Set(j, "PayloadCID", v.payload_cid, allocator);
    Set(j, "ID", v.deal_id, allocator);
    Set(j, "Params", v.params, allocator);
    return j;
  }

  JSON_DECODE(DealProposalV1_0_0) {
    Get(j, "PayloadCID", v.payload_cid);
    Get(j, "ID", v.deal_id);
    Get(j, "Params", v.params);
  }

  JSON_ENCODE(DealProposalParams) {
    Value j{rapidjson::kObjectType};
    Set(j, "Piece", v.piece, allocator);
    Set(j, "PricePerByte", v.price_per_byte, allocator);
    Set(j, "PaymentInterval", v.payment_interval, allocator);
    Set(j, "PaymentIntervalIncrease", v.payment_interval_increase, allocator);
    Set(j, "UnsealPrice", v.unseal_price, allocator);
    return j;
  }

  JSON_DECODE(DealProposalParams) {
    Get(j, "Piece", v.piece);
    Get(j, "PricePerByte", v.price_per_byte);
    Get(j, "PaymentInterval", v.payment_interval);
    Get(j, "PaymentIntervalIncrease", v.payment_interval_increase);
    Get(j, "UnsealPrice", v.unseal_price);
  }

  JSON_ENCODE(RetrievalPeer) {
    Value j{rapidjson::kObjectType};
    Set(j, "Address", v.address, allocator);
    Set(j, "ID", v.peer_id, allocator);
    Set(j, "PieceCID", v.piece, allocator);
    return j;
  }

  JSON_DECODE(RetrievalPeer) {
    Get(j, "Address", v.address);
    Get(j, "ID", v.peer_id);
    Get(j, "PieceCID", v.piece);
  }
}  // namespace fc::markets::retrieval

namespace fc::markets::storage {
  using codec::json::decodeEnum;
  using codec::json::encode;
  using codec::json::Get;
  using codec::json::Set;
  using codec::json::Value;

  namespace provider {
    JSON_ENCODE(MinerDeal) {
      Value j{rapidjson::kObjectType};
      Set(j, "Proposal", v.client_deal_proposal.proposal, allocator);
      Set(j,
          "ClientSignature",
          v.client_deal_proposal.client_signature,
          allocator);
      Set(j, "ProposalCid", v.proposal_cid, allocator);
      Set(j, "AddFundsCid", v.add_funds_cid, allocator);
      Set(j, "PublishCid", v.publish_cid, allocator);
      Set(j, "Client", v.client, allocator);
      Set(j, "State", v.state, allocator);
      Set(j, "PiecePath", v.piece_path, allocator);
      Set(j, "MetadataPath", v.metadata_path, allocator);
      Set(j, "FastRetrieval", v.is_fast_retrieval, allocator);
      Set(j, "Message", v.message, allocator);
      Set(j, "Ref", v.ref, allocator);
      Set(j, "DealId", v.deal_id, allocator);
      return j;
    }

    JSON_DECODE(MinerDeal) {
      Get(j, "Proposal", v.client_deal_proposal.proposal);
      Get(j, "ClientSignature", v.client_deal_proposal.client_signature);
      Get(j, "ProposalCid", v.proposal_cid);
      Get(j, "AddFundsCid", v.add_funds_cid);
      Get(j, "PublishCid", v.publish_cid);
      Get(j, "Client", v.client);
      Get(j, "State", v.state);
      Get(j, "PiecePath", v.piece_path);
      Get(j, "MetadataPath", v.metadata_path);
      Get(j, "FastRetrieval", v.is_fast_retrieval);
      Get(j, "Message", v.message);
      Get(j, "Ref", v.ref);
      Get(j, "DealId", v.deal_id);
    }
  }  // namespace provider
  namespace client::import_manager {
    using codec::json::Get;
    using codec::json::Set;
    using codec::json::Value;

    JSON_ENCODE(Import) {
      Value j{rapidjson::kObjectType};
      Set(j, "StoreId", v.store_id, allocator);
      Set(j, "Error", v.error, allocator);
      Set(j, "Root", v.root, allocator);
      Set(j, "Source", v.source, allocator);
      Set(j, "FilePath", v.path, allocator);
      return j;
    }

    JSON_DECODE(Import) {
      Get(j, "StoreId", v.store_id);
      Get(j, "Error", v.error);
      Get(j, "Root", v.root);
      Get(j, "Source", v.source);
      Get(j, "FilePath", v.path);
    }
  }  // namespace client::import_manager

  JSON_ENCODE(StorageDeal) {
    Value j{rapidjson::kObjectType};
    Set(j, "Proposal", v.proposal, allocator);
    Set(j, "State", v.state, allocator);
    return j;
  }

  JSON_DECODE(StorageDeal) {
    Get(j, "Proposal", v.proposal);
    Get(j, "State", v.state);
  }

  JSON_ENCODE(StorageDealStatus) {
    return encode(common::to_int(v), allocator);
  }

  JSON_DECODE(StorageDealStatus) {
    decodeEnum(v, j);
  }

  JSON_ENCODE(DataRef) {
    Value j{rapidjson::kObjectType};
    Set(j, "TransferType", v.transfer_type, allocator);
    Set(j, "Root", v.root, allocator);
    Set(j, "PieceCid", v.piece_cid, allocator);
    Set(j, "PieceSize", v.piece_size, allocator);
    return j;
  }

  JSON_DECODE(DataRef) {
    Get(j, "TransferType", v.transfer_type);
    Get(j, "Root", v.root);
    Get(j, "PieceCid", v.piece_cid);
    Get(j, "PieceSize", v.piece_size);
  }

  JSON_ENCODE(StorageAsk) {
    Value j{rapidjson::kObjectType};
    Set(j, "Price", v.price, allocator);
    Set(j, "MinPieceSize", v.min_piece_size, allocator);
    Set(j, "Miner", v.miner, allocator);
    Set(j, "Timestamp", v.timestamp, allocator);
    Set(j, "Expiry", v.expiry, allocator);
    Set(j, "SeqNo", v.seq_no, allocator);
    return j;
  }

  JSON_DECODE(StorageAsk) {
    Get(j, "Price", v.price);
    Get(j, "MinPieceSize", v.min_piece_size);
    Get(j, "Miner", v.miner);
    Get(j, "Timestamp", v.timestamp);
    Get(j, "Expiry", v.expiry);
    Get(j, "SeqNo", v.seq_no);
  }

  JSON_ENCODE(SignedStorageAskV1_1_0) {
    Value j{rapidjson::kObjectType};
    Set(j, "Ask", v.ask, allocator);
    Set(j, "Signature", v.signature, allocator);
    return j;
  }

  JSON_DECODE(SignedStorageAskV1_1_0) {
    Get(j, "Ask", v.ask);
    Get(j, "Signature", v.signature);
  }

}  // namespace fc::markets::storage

namespace fc::data_transfer {
  using codec::json::Get;
  using codec::json::Set;
  using codec::json::Value;

  JSON_ENCODE(ChannelId) {
    Value j{rapidjson::kObjectType};
    Set(j, "Initiator", v.initiator, allocator);
    Set(j, "Responder", v.responder, allocator);
    Set(j, "ID", v.id, allocator);
    return j;
  }

  JSON_DECODE(ChannelId) {
    Get(j, "Initiator", v.initiator);
    Get(j, "Responder", v.responder);
    Get(j, "ID", v.id);
  }
}  // namespace fc::data_transfer

namespace fc::api {
  using codec::cbor::CborDecodeStream;
  using codec::json::AsDocument;
  using codec::json::AsString;
  using codec::json::decodeEnum;
  using codec::json::encode;
  using codec::json::Get;
  using codec::json::innerDecode;
  using codec::json::JsonError;
  using codec::json::Set;
  using codec::json::Value;
  using primitives::TaskType;

  JSON_ENCODE(Request) {
    Value j{rapidjson::kObjectType};
    Set(j, "jsonrpc", "2.0", allocator);
    Set(j, "id", v.id, allocator);
    Set(j, "method", v.method, allocator);
    Set(j, "params", Value{v.params, allocator}, allocator);
    return j;
  }

  JSON_DECODE(Request) {
    if (j.HasMember("id")) {
      Get(j, "id", v.id);
    } else {
      v.id = {};
    }
    v.method = AsString(Get(j, "method"));
    v.params = AsDocument(Get(j, "params"));
  }

  JSON_ENCODE(Response) {
    Value j{rapidjson::kObjectType};
    Set(j, "jsonrpc", "2.0", allocator);
    Set(j, "id", v.id, allocator);
    visit_in_place(
        v.result,
        [&](const Response::Error &error) {
          Set(j, "error", error, allocator);
        },
        [&](const Document &result) {
          Set(j, "result", Value{result, allocator}, allocator);
        });
    return j;
  }

  JSON_DECODE(Response) {
    Get(j, "id", v.id);
    if (j.HasMember("error")) {
      v.result = innerDecode<Response::Error>(Get(j, "error"));
    } else if (j.HasMember("result")) {
      v.result = AsDocument(Get(j, "result"));
    } else {
      v.result = Document{};
    }
  }

  JSON_ENCODE(Response::Error) {
    Value j{rapidjson::kObjectType};
    Set(j, "code", v.code, allocator);
    Set(j, "message", v.message, allocator);
    return j;
  }

  JSON_DECODE(Response::Error) {
    Get(j, "code", v.code);
    v.message = AsString(Get(j, "message"));
  }

  JSON_ENCODE(KeyInfo) {
    Value j{rapidjson::kObjectType};
    if (v.type == SignatureType::kBls) {
      Set(j, "Type", "bls", allocator);
    } else if (v.type == SignatureType::kSecp256k1) {
      Set(j, "Type", "secp256k1", allocator);
    } else {
      outcome::raise(JsonError::kWrongEnum);
    }
    Set(j, "PrivateKey", v.private_key, allocator);
    return j;
  }

  JSON_DECODE(KeyInfo) {
    std::string type;
    Get(j, "Type", type);
    Get(j, "PrivateKey", v.private_key);
    if (type == "bls") {
      v.type = SignatureType::kBls;
    } else if (type == "secp256k1") {
      v.type = SignatureType::kSecp256k1;
    } else {
      outcome::raise(JsonError::kWrongEnum);
    }
  }

  JSON_ENCODE(LedgerKeyInfo) {
    Value j{rapidjson::kObjectType};
    Set(j, "Address", v.address, allocator);
    Set(j, "Path", v.path, allocator);
    return j;
  }

  JSON_DECODE(LedgerKeyInfo) {
    Get(j, "Address", v.address);
    Get(j, "Path", v.path);
  }

  JSON_ENCODE(MinerInfo) {
    Value j{rapidjson::kObjectType};
    Set(j, "Owner", v.owner, allocator);
    Set(j, "Worker", v.worker, allocator);
    Set(j, "ControlAddresses", v.control, allocator);
    boost::optional<std::string> peer_id;
    if (!v.peer_id.empty()) {
      OUTCOME_EXCEPT(peer, PeerId::fromBytes(v.peer_id));
      peer_id = peer.toBase58();
    }
    Set(j, "PeerId", peer_id, allocator);
    Set(j, "Multiaddrs", v.multiaddrs, allocator);
    Set(j, "WindowPoStProofType", v.window_post_proof_type, allocator);
    Set(j, "SectorSize", v.sector_size, allocator);
    Set(j,
        "WindowPoStPartitionSectors",
        v.window_post_partition_sectors,
        allocator);
    return j;
  }

  JSON_DECODE(MinerInfo) {
    Get(j, "Owner", v.owner);
    Get(j, "Worker", v.worker);
    Get(j, "ControlAddresses", v.control);
    boost::optional<PeerId> peer_id;
    Get(j, "PeerId", peer_id);
    if (peer_id) {
      v.peer_id = peer_id->toVector();
    } else {
      v.peer_id.clear();
    }
    Get(j, "Multiaddrs", v.multiaddrs);
    Get(j, "WindowPoStProofType", v.window_post_proof_type);
    Get(j, "SectorSize", v.sector_size);
    Get(j, "WindowPoStPartitionSectors", v.window_post_partition_sectors);
  }

  JSON_ENCODE(MessageSendSpec) {
    Value j{rapidjson::kObjectType};
    Set(j, "MaxFee", v.max_fee, allocator);
    return j;
  }

  JSON_DECODE(MessageSendSpec) {
    Get(j, "MaxFee", v.max_fee);
  }

  JSON_ENCODE(MsgWait) {
    Value j{rapidjson::kObjectType};
    Set(j, "Message", v.message, allocator);
    Set(j, "Receipt", v.receipt, allocator);
    Set(j, "TipSet", v.tipset, allocator);
    Set(j, "Height", v.height, allocator);
    return j;
  }

  JSON_DECODE(MsgWait) {
    Get(j, "Message", v.message);
    Get(j, "Receipt", v.receipt);
    Get(j, "TipSet", v.tipset);
    Get(j, "Height", v.height);
  }

  JSON_ENCODE(MinerPower) {
    Value j{rapidjson::kObjectType};
    Set(j, "MinerPower", v.miner, allocator);
    Set(j, "TotalPower", v.total, allocator);
    return j;
  }

  JSON_DECODE(MinerPower) {
    Get(j, "MinerPower", v.miner);
    Get(j, "TotalPower", v.total);
  }

  JSON_ENCODE(MarketBalance) {
    Value j{rapidjson::kObjectType};
    Set(j, "Escrow", v.escrow, allocator);
    Set(j, "Locked", v.locked, allocator);
    return j;
  }

  JSON_DECODE(MarketBalance) {
    Get(j, "Escrow", v.escrow);
    Get(j, "Locked", v.locked);
  }

  JSON_ENCODE(BlockMessages) {
    Value j{rapidjson::kObjectType};
    Set(j, "BlsMessages", v.bls, allocator);
    Set(j, "SecpkMessages", v.secp, allocator);
    Set(j, "Cids", v.cids, allocator);
    return j;
  }

  JSON_DECODE(BlockMessages) {
    Get(j, "BlsMessages", v.bls);
    Get(j, "SecpkMessages", v.secp);
    Get(j, "Cids", v.cids);
  }

  JSON_ENCODE(CidMessage) {
    Value j{rapidjson::kObjectType};
    Set(j, "Cid", v.cid, allocator);
    Set(j, "Message", v.message, allocator);
    return j;
  }

  JSON_DECODE(CidMessage) {
    Get(j, "Cid", v.cid);
    Get(j, "Message", v.message);
  }

  JSON_ENCODE(ApiSectorInfo) {
    Value j{rapidjson::kObjectType};
    Set(j, "SectorID", v.sector_id, allocator);
    Set(j, "State", v.state, allocator);
    Set(j, "CommD", v.comm_d, allocator);
    Set(j, "CommR", v.comm_r, allocator);
    Set(j, "Proof", v.proof, allocator);
    Set(j, "Deals", v.deals, allocator);
    Set(j, "Pieces", v.pieces, allocator);
    Set(j, "Ticket", v.ticket, allocator);
    Set(j, "Seed", v.seed, allocator);
    Set(j, "PreCommitMsg", v.precommit_message, allocator);
    Set(j, "CommitMsg", v.commit_message, allocator);
    Set(j, "Retries", v.retries, allocator);
    Set(j, "ToUpgrade", v.to_upgrade, allocator);
    Set(j, "SealProof", v.seal_proof, allocator);
    Set(j, "Activation", v.activation, allocator);
    Set(j, "Expiration", v.expiration, allocator);
    Set(j, "DealWeight", v.deal_weight, allocator);
    Set(j, "VerifiedDealWeight", v.verified_deal_weight, allocator);
    Set(j, "InitialPledge", v.initial_pledge, allocator);
    Set(j, "OnTime", v.on_time, allocator);
    Set(j, "Early", v.early, allocator);
    return j;
  }

  JSON_DECODE(ApiSectorInfo) {
    Get(j, "SectorID", v.sector_id);
    Get(j, "State", v.state);
    Get(j, "CommD", v.comm_d);
    Get(j, "CommR", v.comm_r);
    Get(j, "Proof", v.proof);
    Get(j, "Deals", v.deals);
    Get(j, "Ticket", v.ticket);
    Get(j, "Pieces", v.pieces);
    Get(j, "Seed", v.seed);
    Get(j, "PreCommitMsg", v.precommit_message);
    Get(j, "CommitMsg", v.commit_message);
    Get(j, "Retries", v.retries);
    Get(j, "ToUpgrade", v.to_upgrade);
    Get(j, "SealProof", v.seal_proof);
    Get(j, "Activation", v.activation);
    Get(j, "Expiration", v.expiration);
    Get(j, "DealWeight", v.deal_weight);
    Get(j, "VerifiedDealWeight", v.verified_deal_weight);
    Get(j, "InitialPledge", v.initial_pledge);
    Get(j, "OnTime", v.on_time);
    Get(j, "Early", v.early);
  }

  JSON_ENCODE(Partition) {
    Value j{rapidjson::kObjectType};
    Set(j, "AllSectors", v.all, allocator);
    Set(j, "FaultySectors", v.faulty, allocator);
    Set(j, "RecoveringSectors", v.recovering, allocator);
    Set(j, "LiveSectors", v.live, allocator);
    Set(j, "ActiveSectors", v.active, allocator);
    return j;
  }

  JSON_DECODE(Partition) {
    Get(j, "AllSectors", v.all);
    Get(j, "FaultySectors", v.faulty);
    Get(j, "RecoveringSectors", v.recovering);
    Get(j, "LiveSectors", v.live);
    Get(j, "ActiveSectors", v.active);
  }

  JSON_ENCODE(Deadline) {
    Value j{rapidjson::kObjectType};
    Set(j, "PostSubmissions", v.partitions_posted, allocator);
    return j;
  }

  JSON_DECODE(Deadline) {
    Get(j, "PostSubmissions", v.partitions_posted);
  }

  JSON_ENCODE(AddChannelInfo) {
    Value j{rapidjson::kObjectType};
    Set(j, "Channel", v.channel, allocator);
    Set(j, "ChannelMessage", v.channel_message, allocator);
    return j;
  }

  JSON_DECODE(AddChannelInfo) {
    Get(j, "Channel", v.channel);
    Get(j, "ChannelMessage", v.channel_message);
  }

  JSON_ENCODE(SectorExpiration) {
    Value j{rapidjson::kObjectType};
    Set(j, "OnTime", v.on_time, allocator);
    Set(j, "Early", v.early, allocator);
    return j;
  }

  JSON_DECODE(SectorExpiration) {
    Get(j, "OnTime", v.on_time);
    Get(j, "Early", v.early);
  }

  JSON_ENCODE(DealCollateralBounds) {
    Value j{rapidjson::kObjectType};
    Set(j, "Min", v.min, allocator);
    Set(j, "Max", v.max, allocator);
    return j;
  }

  JSON_DECODE(DealCollateralBounds) {
    Get(j, "Min", v.min);
    Get(j, "Max", v.max);
  }

  JSON_ENCODE(StartDealParams) {
    Value j{rapidjson::kObjectType};
    Set(j, "Data", v.data, allocator);
    Set(j, "Wallet", v.wallet, allocator);
    Set(j, "Miner", v.miner, allocator);
    Set(j, "EpochPrice", v.epoch_price, allocator);
    Set(j, "MinBlocksDuration", v.min_blocks_duration, allocator);
    Set(j, "ProviderCollateral", v.provider_collateral, allocator);
    Set(j, "DealStartEpoch", v.deal_start_epoch, allocator);
    Set(j, "FastRetrieval", v.fast_retrieval, allocator);
    Set(j, "VerifiedDeal", v.verified_deal, allocator);
    return j;
  }

  JSON_DECODE(StartDealParams) {
    Get(j, "Data", v.data);
    Get(j, "Wallet", v.wallet);
    Get(j, "Miner", v.miner);
    Get(j, "EpochPrice", v.epoch_price);
    Get(j, "MinBlocksDuration", v.min_blocks_duration);
    Get(j, "ProviderCollateral", v.provider_collateral);
    Get(j, "DealStartEpoch", v.deal_start_epoch);
    Get(j, "FastRetrieval", v.fast_retrieval);
    Get(j, "VerifiedDeal", v.verified_deal);
  }

  template <typename T>
  JSON_ENCODE(Chan<T>) {
    return encode(v.id, allocator);
  }

  template <typename T>
  JSON_DECODE(Chan<T>) {
    v.id = innerDecode<uint64_t>(j);
  }

  JSON_ENCODE(InvocResult) {
    Value j{rapidjson::kObjectType};
    Set(j, "Msg", v.message, allocator);
    Set(j, "MsgRct", v.receipt, allocator);
    Set(j, "Error", v.error, allocator);
    return j;
  }

  JSON_DECODE(InvocResult) {
    Get(j, "Msg", v.message);
    Get(j, "MsgRct", v.receipt);
    v.error = AsString(Get(j, "Error"));
  }

  template <typename T>
  inline Value encodeAs(CborDecodeStream &s,
                        rapidjson::MemoryPoolAllocator<> &allocator) {
    T v;
    s >> v;
    return encode(v, allocator);
  }

  inline Value encode(CborDecodeStream &s,
                      rapidjson::MemoryPoolAllocator<> &allocator) {
    if (s.isCid()) {
      return encodeAs<CID>(s, allocator);
    }
    if (s.isList()) {
      auto n = s.listLength();
      Value j{rapidjson::kArrayType};
      j.Reserve(n, allocator);
      auto l = s.list();
      for (; n != 0; --n) {
        j.PushBack(encode(l, allocator), allocator);
      }
      return j;
    }
    if (s.isMap()) {
      auto m = s.map();
      Value j{rapidjson::kObjectType};
      j.MemberReserve(m.size(), allocator);
      for (auto &p : m) {
        Set(j, p.first, encode(p.second, allocator), allocator);
      }
      return j;
    }
    if (s.isNull()) {
      s.next();
      return {};
    }
    if (s.isInt()) {
      return encodeAs<int64_t>(s, allocator);
    }
    if (s.isStr()) {
      return encodeAs<std::string>(s, allocator);
    }
    if (s.isBytes()) {
      return encodeAs<std::vector<uint8_t>>(s, allocator);
    }
    outcome::raise(JsonError::kWrongType);
  }

  JSON_ENCODE(IpldObject) {
    Value j{rapidjson::kObjectType};
    Set(j, "Cid", v.cid, allocator);
    CborDecodeStream s{v.raw};
    Set(j, "Obj", encode(s, allocator), allocator);
    return j;
  }

  JSON_DECODE(IpldObject) {
    outcome::raise(JsonError::kWrongType);
  }

  JSON_ENCODE(ActorState) {
    Value j{rapidjson::kObjectType};
    Set(j, "Balance", v.balance, allocator);
    Set(j, "State", v.state, allocator);
    return j;
  }

  JSON_DECODE(ActorState) {
    // Because IpldObject cannot be decoded
    outcome::raise(JsonError::kWrongType);
  }

  JSON_ENCODE(VersionResult) {
    Value j{rapidjson::kObjectType};
    Set(j, "Version", v.version, allocator);
    Set(j, "APIVersion", v.api_version, allocator);
    Set(j, "BlockDelay", v.block_delay, allocator);
    return j;
  }

  JSON_DECODE(VersionResult) {
    Get(j, "Version", v.version);
    Get(j, "APIVersion", v.api_version);
    Get(j, "BlockDelay", v.block_delay);
  }

  JSON_ENCODE(MiningBaseInfo) {
    Value j{rapidjson::kObjectType};
    Set(j, "MinerPower", v.miner_power, allocator);
    Set(j, "NetworkPower", v.network_power, allocator);
    Set(j, "Sectors", v.sectors, allocator);
    Set(j, "WorkerKey", v.worker, allocator);
    Set(j, "SectorSize", v.sector_size, allocator);
    Set(j, "PrevBeaconEntry", v.prev_beacon, allocator);
    Set(j, "BeaconEntries", v.beacons, allocator);
    Set(j, "EligibleForMining", v.has_min_power, allocator);
    return j;
  }

  JSON_DECODE(MiningBaseInfo) {
    Get(j, "MinerPower", v.miner_power);
    Get(j, "NetworkPower", v.network_power);
    Get(j, "Sectors", v.sectors);
    Get(j, "WorkerKey", v.worker);
    Get(j, "SectorSize", v.sector_size);
    Get(j, "PrevBeaconEntry", v.prev_beacon);
    Get(j, "BeaconEntries", v.beacons);
    Get(j, "EligibleForMining", v.has_min_power);
  }

  JSON_ENCODE(SectorLocation) {
    Value j{rapidjson::kObjectType};
    Set(j, "Deadline", v.deadline, allocator);
    Set(j, "Partition", v.partition, allocator);
    return j;
  }

  JSON_DECODE(SectorLocation) {
    Get(j, "Deadline", v.deadline);
    Get(j, "Partition", v.partition);
  }

  JSON_ENCODE(QueryOffer) {
    Value j{rapidjson::kObjectType};
    Set(j, "Err", v.error, allocator);
    Set(j, "Root", v.root, allocator);
    Set(j, "Piece", v.piece, allocator);
    Set(j, "Size", v.size, allocator);
    Set(j, "MinPrice", v.min_price, allocator);
    Set(j, "UnsealPrice", v.unseal_price, allocator);
    Set(j, "PaymentInterval", v.payment_interval, allocator);
    Set(j, "PaymentIntervalIncrease", v.payment_interval_increase, allocator);
    Set(j, "Miner", v.miner, allocator);
    Set(j, "MinerPeer", v.peer, allocator);
    return j;
  }

  JSON_DECODE(QueryOffer) {
    Get(j, "Err", v.error);
    Get(j, "Root", v.root);
    Get(j, "Piece", v.piece);
    Get(j, "Size", v.size);
    Get(j, "MinPrice", v.min_price);
    Get(j, "UnsealPrice", v.unseal_price);
    Get(j, "PaymentInterval", v.payment_interval);
    Get(j, "PaymentIntervalIncrease", v.payment_interval_increase);
    Get(j, "Miner", v.miner);
    Get(j, "MinerPeer", v.peer);
  }

  JSON_ENCODE(FileRef) {
    Value j{rapidjson::kObjectType};
    Set(j, "Path", v.path, allocator);
    Set(j, "IsCAR", v.is_car, allocator);
    return j;
  }

  JSON_DECODE(FileRef) {
    Get(j, "Path", v.path);
    Get(j, "IsCAR", v.is_car);
  }

  JSON_ENCODE(DatatransferChannel) {
    Value j{rapidjson::kObjectType};
    Set(j, "TransferID", v.transfer_id, allocator);
    Set(j, "Status", v.status, allocator);
    Set(j, "BaseCID", v.base_cid, allocator);
    Set(j, "IsInitiator", v.is_initiator, allocator);
    Set(j, "IsSender", v.is_sender, allocator);
    Set(j, "Voucher", v.voucher, allocator);
    Set(j, "Message", v.message, allocator);
    Set(j, "OtherPeer", v.other_peer, allocator);
    Set(j, "Transferred", v.transferred, allocator);
    return j;
  }

  JSON_DECODE(DatatransferChannel) {
    Get(j, "TransferID", v.transfer_id);
    Get(j, "Status", v.status);
    Get(j, "BaseCID", v.base_cid);
    Get(j, "IsInitiator", v.is_initiator);
    Get(j, "IsSender", v.is_sender);
    Get(j, "Voucher", v.voucher);
    Get(j, "Message", v.message);
    Get(j, "OtherPeer", v.other_peer);
    Get(j, "Transferred", v.transferred);
  }

  JSON_ENCODE(StorageMarketDealInfo) {
    Value j{rapidjson::kObjectType};
    Set(j, "ProposalCid", v.proposal_cid, allocator);
    Set(j, "State", v.state, allocator);
    Set(j, "Message", v.message, allocator);
    Set(j, "Provider", v.provider, allocator);
    Set(j, "DataRef", v.data_ref, allocator);
    Set(j, "PieceCID", v.piece_cid, allocator);
    Set(j, "Size", v.size, allocator);
    Set(j, "PricePerEpoch", v.price_per_epoch, allocator);
    Set(j, "Duration", v.duration, allocator);
    Set(j, "DealID", v.deal_id, allocator);
    // TODO (a.chernyshov) encode and decode time type
    Set(j, "CreationTime", "2006-01-02T15:04:05Z", allocator);
    Set(j, "Verified", v.verified, allocator);
    Set(j, "TransferChannelID", v.transfer_channel_id, allocator);
    Set(j, "DataTransfer", v.data_transfer, allocator);
    return j;
  }

  JSON_DECODE(StorageMarketDealInfo) {
    Get(j, "ProposalCid", v.proposal_cid);
    Get(j, "State", v.state);
    Get(j, "Message", v.message);
    Get(j, "Provider", v.provider);
    Get(j, "DataRef", v.data_ref);
    Get(j, "PieceCID", v.piece_cid);
    Get(j, "Size", v.size);
    Get(j, "PricePerEpoch", v.price_per_epoch);
    Get(j, "Duration", v.duration);
    Get(j, "DealID", v.deal_id);
    // TODO (a.chernyshov) encode and decode time type
    v.creation_time = 0;
    Get(j, "Verified", v.verified);
    Get(j, "TransferChannelID", v.transfer_channel_id);
    Get(j, "DataTransfer", v.data_transfer);
  }

  JSON_ENCODE(ImportRes) {
    Value j{rapidjson::kObjectType};
    Set(j, "Root", v.root, allocator);
    Set(j, "ImportID", v.import_id, allocator);
    return j;
  }

  JSON_DECODE(ImportRes) {
    Get(j, "Root", v.root);
    Get(j, "ImportID", v.import_id);
  }

  JSON_ENCODE(RetrievalOrder) {
    Value j{rapidjson::kObjectType};
    Set(j, "Root", v.root, allocator);
    Set(j, "Piece", v.piece, allocator);
    Set(j, "Size", v.size, allocator);
    Set(j, "LocalStore", v.local_store, allocator);
    Set(j, "Total", v.total, allocator);
    Set(j, "UnsealPrice", v.unseal_price, allocator);
    Set(j, "PaymentInterval", v.payment_interval, allocator);
    Set(j, "PaymentIntervalIncrease", v.payment_interval_increase, allocator);
    Set(j, "Client", v.client, allocator);
    Set(j, "Miner", v.miner, allocator);
    Set(j, "MinerPeer", v.peer, allocator);
    return j;
  }

  JSON_DECODE(RetrievalOrder) {
    Get(j, "Root", v.root);
    Get(j, "Piece", v.piece);
    Get(j, "Size", v.size);
    Get(j, "LocalStore", v.local_store);
    Get(j, "Total", v.total);
    Get(j, "UnsealPrice", v.unseal_price);
    Get(j, "PaymentInterval", v.payment_interval);
    Get(j, "PaymentIntervalIncrease", v.payment_interval_increase);
    Get(j, "Client", v.client);
    Get(j, "Miner", v.miner);
    Get(j, "MinerPeer", v.peer);
  }

}  // namespace fc::api
