/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <rapidjson/document.h>
#include <cppcodec/base64_rfc4648.hpp>

#include "api/full_node/node_api.hpp"
#include "api/rpc/json_errors.hpp"
#include "api/rpc/rpc.hpp"
#include "api/storage_miner/storage_api.hpp"
#include "api/types/key_info.hpp"
#include "api/worker_api.hpp"
#include "common/enum.hpp"
#include "common/libp2p/peer/cbor_peer_info.hpp"
#include "miner/main/type.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "sector_storage/stores/storage.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"
#include "vm/actor/builtin/types/miner/miner_info.hpp"

#define COMMA ,

#define ENCODE(type) Value encode(const type &v)

// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define DECODE(type) static void decode(type &v, const Value &j)

namespace fc::codec::cbor {
  using markets::storage::StorageDealStatus;

  template <>
  inline StorageDealStatus kDefaultT<StorageDealStatus>() {
    return StorageDealStatus::STORAGE_DEAL_UNKNOWN;
  }

}  // namespace fc::codec::cbor

namespace fc::api {
  using codec::cbor::CborDecodeStream;
  using common::Blob;
  using crypto::signature::BlsSignature;
  using crypto::signature::Secp256k1Signature;
  using crypto::signature::Signature;
  using markets::storage::StorageAsk;
  using miner::types::PreSealSector;
  using mining::SealingState;
  using primitives::BigInt;
  using primitives::FsStat;
  using primitives::kChainEpochUndefined;
  using primitives::LocalStorageMeta;
  using primitives::StoragePath;
  using primitives::TaskType;
  using primitives::WorkerInfo;
  using primitives::WorkerResources;
  using primitives::block::ElectionProof;
  using primitives::block::Ticket;
  using primitives::cid::getCidOfCbor;
  using primitives::piece::ReaderType;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::PoStProof;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::SectorId;
  using primitives::sector::SectorRef;
  using primitives::tipset::HeadChangeType;
  using proofs::SealedAndUnsealedCID;
  using rapidjson::Document;
  using rapidjson::Value;
  using sector_storage::CallErrorCode;
  using sector_storage::CallId;
  using sector_storage::Range;
  using sector_storage::SectorFileType;
  using sector_storage::stores::AcquireMode;
  using sector_storage::stores::LocalPath;
  using sector_storage::stores::PathType;
  using sector_storage::stores::StorageConfig;
  using sector_storage::stores::StorageInfo;
  using vm::actor::builtin::types::market::DealProposal;
  using vm::actor::builtin::types::market::DealState;
  using vm::actor::builtin::types::market::StorageParticipantBalance;
  using vm::actor::builtin::types::miner::PowerPair;
  using vm::actor::builtin::types::miner::SectorPreCommitInfo;
  using vm::actor::builtin::types::miner::WorkerKeyChange;
  using vm::actor::builtin::types::payment_channel::Merge;
  using vm::actor::builtin::types::payment_channel::
      ModularVerificationParameter;
  using vm::runtime::ExecutionResult;
  using base64 = cppcodec::base64_rfc4648;

  struct Codec {
    rapidjson::MemoryPoolAllocator<> &allocator;

    template <typename T>
    static void decodeEnum(T &v, const Value &j) {
      v = T{decode<std::underlying_type_t<T>>(j)};
    }

    static std::string AsString(const Value &j) {
      if (!j.IsString()) {
        outcome::raise(JsonError::kWrongType);
      }
      return {j.GetString(), j.GetStringLength()};
    }

    static const Value &Get(const Value &j, const char *key) {
      if (!j.IsObject()) {
        outcome::raise(JsonError::kWrongType);
      }
      auto it = j.FindMember(key);
      if (it == j.MemberEnd()) {
        outcome::raise(JsonError::kOutOfRange);
      }
      return it->value;
    }

    void Set(Value &j, std::string_view key, Value &&value) {
      j.AddMember(encode(key), value, allocator);
    }

    template <typename T>
    void Set(Value &j, std::string_view key, const T &v) {
      Set(j, key, encode(v));
    }

    template <typename T>
    static void Get(const Value &j, const char *key, T &v) {
      decode(v, Get(j, key));
    }

    static std::vector<uint8_t> decodeBase64(const Value &j) {
      if (j.IsNull()) {
        return {};
      }
      return base64::decode(AsString(j));
    }

    static auto AsDocument(const Value &j) {
      Document document;
      static_cast<Value &>(document) = Value{j, document.GetAllocator()};
      return document;
    }

    DECODE(Document) {
      v = AsDocument(j);
    }

    ENCODE(Request) {
      Value j{rapidjson::kObjectType};
      Set(j, "jsonrpc", "2.0");
      Set(j, "id", v.id);
      Set(j, "method", v.method);
      Set(j, "params", Value{v.params, allocator});
      return j;
    }

    DECODE(Request) {
      if (j.HasMember("id")) {
        Get(j, "id", v.id);
      } else {
        v.id = {};
      }
      v.method = AsString(Get(j, "method"));
      v.params = AsDocument(Get(j, "params"));
    }

    ENCODE(Response) {
      Value j{rapidjson::kObjectType};
      Set(j, "jsonrpc", "2.0");
      Set(j, "id", v.id);
      visit_in_place(
          v.result,
          [&](const Response::Error &error) { Set(j, "error", error); },
          [&](const Document &result) {
            Set(j, "result", Value{result, allocator});
          });
      return j;
    }

    DECODE(Response) {
      decode(v.id, Get(j, "id"));
      if (j.HasMember("error")) {
        v.result = decode<Response::Error>(Get(j, "error"));
      } else if (j.HasMember("result")) {
        v.result = AsDocument(Get(j, "result"));
      } else {
        v.result = Document{};
      }
    }

    ENCODE(Response::Error) {
      Value j{rapidjson::kObjectType};
      Set(j, "code", v.code);
      Set(j, "message", v.message);
      return j;
    }

    DECODE(Response::Error) {
      decode(v.code, Get(j, "code"));
      v.message = AsString(Get(j, "message"));
    }

    template <typename T, typename = std::enable_if_t<std::is_same_v<T, bool>>>
    ENCODE(T) {
      return Value{v};
    }

    ENCODE(PaddedPieceSize) {
      return encode(static_cast<uint64_t>(v));
    }

    DECODE(PaddedPieceSize) {
      v = decode<uint64_t>(j);
    }

    ENCODE(UnpaddedPieceSize) {
      return encode(static_cast<uint64_t>(v));
    }

    DECODE(UnpaddedPieceSize) {
      v = decode<uint64_t>(j);
    }

    ENCODE(RegisteredSealProof) {
      return encode(common::to_int(v));
    }

    DECODE(RegisteredSealProof) {
      decodeEnum(v, j);
    }

    ENCODE(RegisteredPoStProof) {
      return encode(common::to_int(v));
    }

    DECODE(RegisteredPoStProof) {
      decodeEnum(v, j);
    }

    ENCODE(SectorFileType) {
      return encode(common::to_int(v));
    }

    DECODE(SectorFileType) {
      decodeEnum(v, j);
    }

    ENCODE(SealingState) {
      return encode(common::to_int(v));
    }

    DECODE(SealingState) {
      decodeEnum(v, j);
    }

    ENCODE(PathType) {
      return encode(common::to_int(v));
    }

    DECODE(PathType) {
      decodeEnum(v, j);
    }

    ENCODE(AcquireMode) {
      return encode(common::to_int(v));
    }

    DECODE(AcquireMode) {
      decodeEnum(v, j);
    }

    ENCODE(NetworkVersion) {
      return encode(common::to_int(v));
    }

    DECODE(NetworkVersion) {
      decodeEnum(v, j);
    }

    ENCODE(StorageDealStatus) {
      return encode(common::to_int(v));
    }

    DECODE(StorageDealStatus) {
      decodeEnum(v, j);
    }

    ENCODE(int64_t) {
      return Value{v};
    }

    DECODE(int64_t) {
      if (j.IsInt64()) {
        v = j.GetInt64();
      } else if (j.IsString()) {
        v = strtoll(j.GetString(), nullptr, 10);
      } else {
        outcome::raise(JsonError::kWrongType);
      }
    }

    ENCODE(uint64_t) {
      return Value{v};
    }

    DECODE(uint64_t) {
      if (j.IsUint64()) {
        v = j.GetUint64();
      } else if (j.IsString()) {
        v = strtoull(j.GetString(), nullptr, 10);
      } else {
        outcome::raise(JsonError::kWrongType);
      }
    }

    ENCODE(double) {
      return Value{v};
    }

    DECODE(double) {
      if (j.IsDouble()) {
        v = j.GetDouble();
      } else if (j.IsString()) {
        v = strtod(j.GetString(), nullptr);
      } else {
        outcome::raise(JsonError::kWrongType);
      }
    }

    DECODE(bool) {
      if (!j.IsBool()) {
        outcome::raise(JsonError::kWrongType);
      }
      v = j.GetBool();
    }

    ENCODE(std::string_view) {
      return {v.data(), static_cast<rapidjson::SizeType>(v.size()), allocator};
    }

    DECODE(std::string) {
      v = AsString(j);
    }

    ENCODE(TaskType) {
      return encode(std::string_view(v));
    }

    DECODE(TaskType) {
      v = TaskType(AsString(j));
    }

    ENCODE(gsl::span<const uint8_t>) {
      return encode(base64::encode(v.data(), v.size()));
    }

    template <size_t N>
    DECODE(std::array<uint8_t COMMA N>) {
      auto bytes = decodeBase64(j);
      if (bytes.size() != N) {
        outcome::raise(JsonError::kWrongLength);
      }
      std::copy(bytes.begin(), bytes.end(), v.begin());
    }

    DECODE(std::vector<uint8_t>) {
      v = decodeBase64(j);
    }

    ENCODE(CID) {
      OUTCOME_EXCEPT(str, v.toString());
      Value j{rapidjson::kObjectType};
      Set(j, "/", str);
      return j;
    }

    DECODE(CID) {
      OUTCOME_EXCEPT(cid, CID::fromString(AsString(Get(j, "/"))));
      v = std::move(cid);
    }

    ENCODE(CbCid) {
      return encode(CID{v});
    }

    DECODE(CbCid) {
      if (auto cid{asBlake(decode<CID>(j))}) {
        v = *cid;
      } else {
        outcome::raise(JsonError::kWrongType);
      }
    }

    ENCODE(PeerId) {
      return encode(v.toBase58());
    }

    DECODE(PeerId) {
      OUTCOME_EXCEPT(id, PeerId::fromBase58(AsString(j)));
      v = std::move(id);
    }

    ENCODE(Ticket) {
      Value j{rapidjson::kObjectType};
      Set(j, "VRFProof", gsl::make_span(v.bytes));
      return j;
    }

    DECODE(Ticket) {
      decode(v.bytes, Get(j, "VRFProof"));
    }

    ENCODE(BlockParentCbCids) {
      if (v.mainnet_genesis) {
        static const std::vector<CID> mainnet{
            CID::fromBytes(kMainnetGenesisBlockParent).value()};
        return encode(mainnet);
      }
      return encode<std::vector<CbCid>>(v);
    }

    DECODE(BlockParentCbCids) {
      const auto cids{decode<std::vector<CID>>(j)};
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

    ENCODE(TipsetKey) {
      return encode(v.cids());
    }

    DECODE(TipsetKey) {
      v = decode<std::vector<CbCid>>(j);
    }

    ENCODE(Address) {
      return encode(primitives::address::encodeToString(v));
    }

    DECODE(Address) {
      OUTCOME_EXCEPT(decoded,
                     primitives::address::decodeFromString(AsString(j)));
      v = std::move(decoded);
    }

    ENCODE(Signature) {
      uint64_t type = SignatureType::kUndefined;
      gsl::span<const uint8_t> data;
      visit_in_place(
          v,
          [&](const BlsSignature &bls) {
            type = SignatureType::kBls;
            data = gsl::make_span(bls);
          },
          [&](const Secp256k1Signature &secp) {
            type = SignatureType::kSecp256k1;
            data = gsl::make_span(secp);
          });
      Value j{rapidjson::kObjectType};
      Set(j, "Type", type);
      Set(j, "Data", data);
      return j;
    }

    DECODE(Signature) {
      uint64_t type = SignatureType::kUndefined;
      decode(type, Get(j, "Type"));
      const auto &data = Get(j, "Data");
      if (type == SignatureType::kBls) {
        v = decode<BlsSignature>(data);
      } else if (type == SignatureType::kSecp256k1) {
        v = decode<Secp256k1Signature>(data);
      } else {
        outcome::raise(JsonError::kWrongEnum);
      }
    }

    ENCODE(KeyInfo) {
      Value j{rapidjson::kObjectType};
      if (v.type == SignatureType::kBls) {
        Set(j, "Type", "bls");
      } else if (v.type == SignatureType::kSecp256k1) {
        Set(j, "Type", "secp256k1");
      } else {
        outcome::raise(JsonError::kWrongEnum);
      }
      Set(j, "PrivateKey", v.private_key);
      return j;
    }

    DECODE(KeyInfo) {
      std::string type;
      decode(type, Get(j, "Type"));
      decode(v.private_key, Get(j, "PrivateKey"));
      if (type == "bls") {
        v.type = SignatureType::kBls;
      } else if (type == "secp256k1") {
        v.type = SignatureType::kSecp256k1;
      } else {
        outcome::raise(JsonError::kWrongEnum);
      }
    }

    ENCODE(PoStProof) {
      Value j{rapidjson::kObjectType};
      Set(j, "PoStProof", v.registered_proof);
      Set(j, "ProofBytes", gsl::make_span(v.proof));
      return j;
    }

    DECODE(PoStProof) {
      Get(j, "PoStProof", v.registered_proof);
      decode(v.proof, Get(j, "ProofBytes"));
    }

    ENCODE(CallErrorCode) {
      return encode(common::to_int(v));
    }

    DECODE(CallErrorCode) {
      decodeEnum(v, j);
    }

    ENCODE(CallError) {
      Value j{rapidjson::kObjectType};
      Set(j, "Code", v.code);
      Set(j, "ProofBytes", v.message);
      return j;
    }

    DECODE(CallError) {
      Get(j, "Code", v.code);
      Get(j, "Message", v.message);
    }

    ENCODE(BigInt) {
      return encode(boost::lexical_cast<std::string>(v));
    }

    DECODE(BigInt) {
      v = BigInt{AsString(j)};
    }

    ENCODE(MinerDeal) {
      Value j{rapidjson::kObjectType};
      Set(j, "Proposal", v.client_deal_proposal.proposal);
      Set(j, "ClientSignature", v.client_deal_proposal.client_signature);
      Set(j, "ProposalCid", v.proposal_cid);
      Set(j, "AddFundsCid", v.add_funds_cid);
      Set(j, "PublishCid", v.publish_cid);
      Set(j, "Client", v.client);
      Set(j, "State", v.state);
      Set(j, "PiecePath", v.piece_path);
      Set(j, "MetadataPath", v.metadata_path);
      Set(j, "FastRetrieval", v.is_fast_retrieval);
      Set(j, "Message", v.message);
      Set(j, "Ref", v.ref);
      Set(j, "DealId", v.deal_id);
      return j;
    }

    DECODE(MinerDeal) {
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

    ENCODE(MinerInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "Owner", v.owner);
      Set(j, "Worker", v.worker);
      Set(j, "ControlAddresses", v.control);
      boost::optional<std::string> peer_id;
      if (!v.peer_id.empty()) {
        OUTCOME_EXCEPT(peer, PeerId::fromBytes(v.peer_id));
        peer_id = peer.toBase58();
      }
      Set(j, "PeerId", peer_id);
      Set(j, "Multiaddrs", v.multiaddrs);
      Set(j, "WindowPoStProofType", v.window_post_proof_type);
      Set(j, "SectorSize", v.sector_size);
      Set(j, "WindowPoStPartitionSectors", v.window_post_partition_sectors);
      return j;
    }

    DECODE(MinerInfo) {
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

    ENCODE(WorkerKeyChange) {
      Value j{rapidjson::kObjectType};
      Set(j, "NewWorker", v.new_worker);
      Set(j, "EffectiveAt", v.effective_at);
      return j;
    }

    ENCODE(DeadlineInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "CurrentEpoch", v.current_epoch);
      Set(j, "PeriodStart", v.period_start);
      Set(j, "Index", v.index);
      Set(j, "Open", v.open);
      Set(j, "Close", v.close);
      Set(j, "Challenge", v.challenge);
      Set(j, "FaultCutoff", v.fault_cutoff);
      Set(j, "WPoStPeriodDeadlines", v.wpost_period_deadlines);
      Set(j, "WPoStProvingPeriod", v.wpost_proving_period);
      Set(j, "WPoStChallengeWindow", v.wpost_challenge_window);
      Set(j, "WPoStChallengeLookback", v.wpost_challenge_lookback);
      Set(j, "FaultDeclarationCutoff", v.fault_declaration_cutoff);
      return j;
    }

    DECODE(DeadlineInfo) {
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

    ENCODE(DomainSeparationTag) {
      return encode(common::to_int(v));
    }

    DECODE(DomainSeparationTag) {
      decodeEnum(v, j);
    }

    ENCODE(MessageSendSpec) {
      Value j{rapidjson::kObjectType};
      Set(j, "MaxFee", v.max_fee);
      return j;
    }

    DECODE(MessageSendSpec) {
      Get(j, "MaxFee", v.max_fee);
    }

    ENCODE(BlockHeader) {
      Value j{rapidjson::kObjectType};
      Set(j, "Miner", v.miner);
      Set(j, "Ticket", v.ticket);
      Set(j, "ElectionProof", v.election_proof);
      Set(j, "BeaconEntries", v.beacon_entries);
      Set(j, "WinPoStProof", v.win_post_proof);
      Set(j, "Parents", v.parents);
      Set(j, "ParentWeight", v.parent_weight);
      Set(j, "Height", v.height);
      Set(j, "ParentStateRoot", v.parent_state_root);
      Set(j, "ParentMessageReceipts", v.parent_message_receipts);
      Set(j, "Messages", v.messages);
      Set(j, "BLSAggregate", v.bls_aggregate);
      Set(j, "Timestamp", v.timestamp);
      Set(j, "BlockSig", v.block_sig);
      Set(j, "ForkSignaling", v.fork_signaling);
      Set(j, "ParentBaseFee", v.parent_base_fee);
      return j;
    }

    DECODE(BlockHeader) {
      decode(v.miner, Get(j, "Miner"));
      decode(v.ticket, Get(j, "Ticket"));
      decode(v.election_proof, Get(j, "ElectionProof"));
      decode(v.beacon_entries, Get(j, "BeaconEntries"));
      decode(v.win_post_proof, Get(j, "WinPoStProof"));
      decode(v.parents, Get(j, "Parents"));
      decode(v.parent_weight, Get(j, "ParentWeight"));
      decode(v.height, Get(j, "Height"));
      decode(v.parent_state_root, Get(j, "ParentStateRoot"));
      decode(v.parent_message_receipts, Get(j, "ParentMessageReceipts"));
      decode(v.messages, Get(j, "Messages"));
      decode(v.bls_aggregate, Get(j, "BLSAggregate"));
      decode(v.timestamp, Get(j, "Timestamp"));
      decode(v.block_sig, Get(j, "BlockSig"));
      decode(v.fork_signaling, Get(j, "ForkSignaling"));
      decode(v.parent_base_fee, Get(j, "ParentBaseFee"));
    }

    ENCODE(BlockTemplate) {
      Value j{rapidjson::kObjectType};
      Set(j, "Miner", v.miner);
      Set(j, "Parents", v.parents);
      Set(j, "Ticket", v.ticket);
      Set(j, "Eproof", v.election_proof);
      Set(j, "BeaconValues", v.beacon_entries);
      Set(j, "Messages", v.messages);
      Set(j, "Epoch", v.height);
      Set(j, "Timestamp", v.timestamp);
      Set(j, "WinningPoStProof", v.win_post_proof);
      return j;
    }

    DECODE(BlockTemplate) {
      decode(v.miner, Get(j, "Miner"));
      decode(v.parents, Get(j, "Parents"));
      decode(v.ticket, Get(j, "Ticket"));
      decode(v.election_proof, Get(j, "Eproof"));
      decode(v.beacon_entries, Get(j, "BeaconValues"));
      decode(v.messages, Get(j, "Messages"));
      decode(v.height, Get(j, "Epoch"));
      decode(v.timestamp, Get(j, "Timestamp"));
      decode(v.win_post_proof, Get(j, "WinningPoStProof"));
    }

    ENCODE(ElectionProof) {
      Value j{rapidjson::kObjectType};
      Set(j, "WinCount", v.win_count);
      Set(j, "VRFProof", v.vrf_proof);
      return j;
    }

    DECODE(ElectionProof) {
      decode(v.win_count, Get(j, "WinCount"));
      decode(v.vrf_proof, Get(j, "VRFProof"));
    }

    ENCODE(BeaconEntry) {
      Value j{rapidjson::kObjectType};
      Set(j, "Round", v.round);
      Set(j, "Data", v.data);
      return j;
    }

    DECODE(BeaconEntry) {
      decode(v.round, Get(j, "Round"));
      decode(v.data, Get(j, "Data"));
    }

    ENCODE(TipsetCPtr) {
      Value j{rapidjson::kObjectType};
      Set(j, "Cids", v->key.cids());
      Set(j, "Blocks", v->blks);
      Set(j, "Height", v->height());
      return j;
    }

    DECODE(TipsetCPtr) {
      std::vector<primitives::block::BlockHeader> blks;
      decode(blks, Get(j, "Blocks"));

      // TODO (a.gorbachev) decode and verify excessive values

      OUTCOME_EXCEPT(tipset,
                     primitives::tipset::Tipset::create(std::move(blks)));
      v = std::move(tipset);
    }

    ENCODE(MessageReceipt) {
      Value j{rapidjson::kObjectType};
      Set(j, "ExitCode", common::to_int(v.exit_code));
      Set(j, "Return", gsl::make_span(v.return_value));
      Set(j, "GasUsed", v.gas_used);
      return j;
    }

    DECODE(MessageReceipt) {
      decodeEnum(v.exit_code, Get(j, "ExitCode"));
      decode(v.return_value, Get(j, "Return"));
      decode(v.gas_used, Get(j, "GasUsed"));
    }

    ENCODE(MsgWait) {
      Value j{rapidjson::kObjectType};
      Set(j, "Message", v.message);
      Set(j, "Receipt", v.receipt);
      Set(j, "TipSet", v.tipset);
      Set(j, "Height", v.height);
      return j;
    }

    DECODE(MsgWait) {
      decode(v.message, Get(j, "Message"));
      decode(v.receipt, Get(j, "Receipt"));
      decode(v.tipset, Get(j, "TipSet"));
      decode(v.height, Get(j, "Height"));
    }

    ENCODE(MpoolUpdate) {
      Value j{rapidjson::kObjectType};
      Set(j, "Type", common::to_int(v.type));
      Set(j, "Message", v.message);
      return j;
    }

    DECODE(MpoolUpdate) {
      decodeEnum(v.type, Get(j, "Type"));
      decode(v.message, Get(j, "Message"));
    }

    ENCODE(MinerPower) {
      Value j{rapidjson::kObjectType};
      Set(j, "MinerPower", v.miner);
      Set(j, "TotalPower", v.total);
      return j;
    }

    DECODE(MinerPower) {
      decode(v.miner, Get(j, "MinerPower"));
      decode(v.total, Get(j, "TotalPower"));
    }

    ENCODE(Claim) {
      Value j{rapidjson::kObjectType};
      Set(j, "RawBytePower", v.raw_power);
      Set(j, "QualityAdjPower", v.qa_power);
      return j;
    }

    DECODE(Claim) {
      decode(v.raw_power, Get(j, "RawBytePower"));
      decode(v.qa_power, Get(j, "QualityAdjPower"));
    }

    ENCODE(MarketBalance) {
      Value j{rapidjson::kObjectType};
      Set(j, "Escrow", v.escrow);
      Set(j, "Locked", v.locked);
      return j;
    }

    DECODE(MarketBalance) {
      Get(j, "Escrow", v.escrow);
      Get(j, "Locked", v.locked);
    }

    ENCODE(StorageInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "ID", v.id);
      Set(j, "URLs", v.urls);
      Set(j, "Weight", v.weight);
      Set(j, "CanSeal", v.can_seal);
      Set(j, "CanStore", v.can_store);
      Set(j, "Primary", v.is_primary);
      return j;
    }

    DECODE(StorageInfo) {
      Get(j, "ID", v.id);
      Get(j, "URLs", v.urls);
      Get(j, "Weight", v.weight);
      Get(j, "CanSeal", v.can_seal);
      Get(j, "CanStore", v.can_store);
      Get(j, "Primary", v.is_primary);
    }

    ENCODE(StorageParticipantBalance) {
      Value j{rapidjson::kObjectType};
      Set(j, "Locked", v.locked);
      Set(j, "Available", v.available);
      return j;
    }

    DECODE(StorageParticipantBalance) {
      decode(v.locked, Get(j, "Locked"));
      decode(v.available, Get(j, "Available"));
    }

    ENCODE(StoragePath) {
      Value j{rapidjson::kObjectType};
      Set(j, "ID", v.id);
      Set(j, "Weight", v.weight);
      Set(j, "LocalPath", v.local_path);
      Set(j, "CanSeal", v.can_seal);
      Set(j, "CanStore", v.can_store);
      return j;
    }

    DECODE(StoragePath) {
      decode(v.id, Get(j, "ID"));
      decode(v.weight, Get(j, "Weight"));
      decode(v.local_path, Get(j, "LocalPath"));
      decode(v.can_seal, Get(j, "CanSeal"));
      decode(v.can_store, Get(j, "CanStore"));
    }

    ENCODE(Range) {
      Value j{rapidjson::kObjectType};
      Set(j, "Offset", v.offset);
      Set(j, "Size", v.size);
      return j;
    }

    DECODE(Range) {
      decode(v.offset, Get(j, "Offset"));
      decode(v.size, Get(j, "Size"));
    }

    ENCODE(DealSchedule) {
      Value j{rapidjson::kObjectType};
      Set(j, "StartEpoch", v.start_epoch);
      Set(j, "EndEpoch", v.end_epoch);
      return j;
    }

    DECODE(DealSchedule) {
      Get(j, "StartEpoch", v.end_epoch);
      Get(j, "EndEpoch", v.end_epoch);
    }

    ENCODE(DealInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "PublishCid", v.publish_cid);
      Set(j, "DealID", v.deal_id);
      Set(j, "DealProposal", v.deal_proposal);
      Set(j, "DealSchedule", v.deal_schedule);
      Set(j, "KeepUnsealed", v.is_keep_unsealed);
      return j;
    }

    DECODE(DealInfo) {
      Get(j, "PublishCid", v.publish_cid);
      Get(j, "DealID", v.deal_id);
      Get(j, "DealProposal", v.deal_proposal);
      Get(j, "DealSchedule", v.deal_schedule);
      Get(j, "KeepUnsealed", v.is_keep_unsealed);
    }

    ENCODE(Piece) {
      Value j{rapidjson::kObjectType};
      Set(j, "Piece", v.piece);
      Set(j, "DealInfo", v.deal_info);
      return j;
    }

    DECODE(Piece) {
      Get(j, "Piece", v.piece);
      Get(j, "DealInfo", v.deal_info);
    }

    ENCODE(SealedAndUnsealedCID) {
      Value j{rapidjson::kObjectType};
      Set(j, "Sealed", v.sealed_cid);
      Set(j, "Unsealed", v.unsealed_cid);
      return j;
    }

    DECODE(SealedAndUnsealedCID) {
      decode(v.sealed_cid, Get(j, "Sealed"));
      decode(v.unsealed_cid, Get(j, "Unsealed"));
    }

    ENCODE(RleBitset) {
      return encode(codec::rle::toRuns(v));
    }

    DECODE(RleBitset) {
      v = codec::rle::fromRuns(decode<codec::rle::Runs64>(j));
    }

    ENCODE(UnsignedMessage) {
      Value j{rapidjson::kObjectType};
      Set(j, "Version", v.version);
      Set(j, "To", v.to);
      Set(j, "From", v.from);
      Set(j, "Nonce", v.nonce);
      Set(j, "Value", v.value);
      Set(j, "GasLimit", v.gas_limit);
      Set(j, "GasFeeCap", v.gas_fee_cap);
      Set(j, "GasPremium", v.gas_premium);
      Set(j, "Method", v.method);
      Set(j, "Params", gsl::make_span(v.params));
      return j;
    }

    DECODE(UnsignedMessage) {
      decode(v.version, Get(j, "Version"));
      decode(v.to, Get(j, "To"));
      decode(v.from, Get(j, "From"));
      decode(v.nonce, Get(j, "Nonce"));
      decode(v.value, Get(j, "Value"));
      decode(v.gas_limit, Get(j, "GasLimit"));
      decode(v.gas_fee_cap, Get(j, "GasFeeCap"));
      decode(v.gas_premium, Get(j, "GasPremium"));
      decode(v.method, Get(j, "Method"));
      decode(v.params, Get(j, "Params"));
    }

    ENCODE(SignedMessage) {
      Value j{rapidjson::kObjectType};
      Set(j, "Message", v.message);
      Set(j, "Signature", v.signature);
      OUTCOME_EXCEPT(
          cid, v.signature.isBls() ? getCidOfCbor(v.message) : getCidOfCbor(v));
      Set(j, "CID", cid);
      return j;
    }

    DECODE(SignedMessage) {
      decode(v.message, Get(j, "Message"));
      decode(v.signature, Get(j, "Signature"));
    }

    ENCODE(StorageConfig) {
      Value j{rapidjson::kObjectType};
      Set(j, "StoragePaths", v.storage_paths);
      return j;
    }

    DECODE(StorageConfig) {
      decode(v.storage_paths, Get(j, "StoragePaths"));
    }

    ENCODE(LocalPath) {
      Value j{rapidjson::kObjectType};
      Set(j, "Path", v.path);
      return j;
    }

    DECODE(LocalPath) {
      decode(v.path, Get(j, "Path"));
    }

    ENCODE(BlockMessages) {
      Value j{rapidjson::kObjectType};
      Set(j, "BlsMessages", v.bls);
      Set(j, "SecpkMessages", v.secp);
      Set(j, "Cids", v.cids);
      return j;
    }

    DECODE(BlockMessages) {
      decode(v.bls, Get(j, "BlsMessages"));
      decode(v.secp, Get(j, "SecpkMessages"));
      decode(v.cids, Get(j, "Cids"));
    }

    ENCODE(CidMessage) {
      Value j{rapidjson::kObjectType};
      Set(j, "Cid", v.cid);
      Set(j, "Message", v.message);
      return j;
    }

    DECODE(CidMessage) {
      decode(v.cid, Get(j, "Cid"));
      decode(v.message, Get(j, "Message"));
    }

    ENCODE(SectorInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "SealProof", v.registered_proof);
      Set(j, "SectorNumber", v.sector);
      Set(j, "SealedCID", v.sealed_cid);
      return j;
    }

    DECODE(SectorInfo) {
      Get(j, "SealProof", v.registered_proof);
      Get(j, "SectorNumber", v.sector);
      Get(j, "SealedCID", v.sealed_cid);
    }

    ENCODE(ApiSectorInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "SectorID", v.sector_id);
      Set(j, "State", v.state);
      Set(j, "CommD", v.comm_d);
      Set(j, "CommR", v.comm_r);
      Set(j, "Proof", v.proof);
      Set(j, "Deals", v.deals);
      Set(j, "Pieces", v.pieces);
      Set(j, "Ticket", v.ticket);
      Set(j, "Seed", v.seed);
      Set(j, "PreCommitMsg", v.precommit_message);
      Set(j, "CommitMsg", v.commit_message);
      Set(j, "Retries", v.retries);
      Set(j, "ToUpgrade", v.to_upgrade);
      Set(j, "SealProof", v.seal_proof);
      Set(j, "Activation", v.activation);
      Set(j, "Expiration", v.expiration);
      Set(j, "DealWeight", v.deal_weight);
      Set(j, "VerifiedDealWeight", v.verified_deal_weight);
      Set(j, "InitialPledge", v.initial_pledge);
      Set(j, "OnTime", v.on_time);
      Set(j, "Early", v.early);
      return j;
    }

    DECODE(ApiSectorInfo) {
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

    ENCODE(PowerPair) {
      Value j{rapidjson::kObjectType};
      Set(j, "Raw", v.raw);
      Set(j, "QA", v.qa);
      return j;
    }

    DECODE(PowerPair) {
      Get(j, "Raw", v.raw);
      Get(j, "QA", v.qa);
    }

    ENCODE(Partition) {
      Value j{rapidjson::kObjectType};
      Set(j, "AllSectors", v.all);
      Set(j, "FaultySectors", v.faulty);
      Set(j, "RecoveringSectors", v.recovering);
      Set(j, "LiveSectors", v.live);
      Set(j, "ActiveSectors", v.active);
      return j;
    }

    DECODE(Partition) {
      Get(j, "AllSectors", v.all);
      Get(j, "FaultySectors", v.faulty);
      Get(j, "RecoveringSectors", v.recovering);
      Get(j, "LiveSectors", v.live);
      Get(j, "ActiveSectors", v.active);
    }

    ENCODE(Deadline) {
      Value j{rapidjson::kObjectType};
      Set(j, "PostSubmissions", v.partitions_posted);
      return j;
    }

    DECODE(Deadline) {
      Get(j, "PostSubmissions", v.partitions_posted);
    }

    ENCODE(SectorPreCommitInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "SealProof", v.registered_proof);
      Set(j, "SectorNumber", v.sector);
      Set(j, "SealedCID", v.sealed_cid);
      Set(j, "SealRandEpoch", v.seal_epoch);
      Set(j, "DealIDs", v.deal_ids);
      Set(j, "Expiration", v.expiration);
      return j;
    }

    DECODE(SectorPreCommitInfo) {
      Get(j, "SealProof", v.registered_proof);
      decode(v.sector, Get(j, "SectorNumber"));
      decode(v.sealed_cid, Get(j, "SealedCID"));
      decode(v.seal_epoch, Get(j, "SealRandEpoch"));
      decode(v.deal_ids, Get(j, "DealIDs"));
      decode(v.expiration, Get(j, "Expiration"));
    }

    ENCODE(SectorPreCommitOnChainInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "Info", v.info);
      Set(j, "PreCommitDeposit", v.precommit_deposit);
      Set(j, "PreCommitEpoch", v.precommit_epoch);
      Set(j, "DealWeight", v.deal_weight);
      Set(j, "VerifiedDealWeight", v.verified_deal_weight);
      return j;
    }

    DECODE(SectorPreCommitOnChainInfo) {
      decode(v.info, Get(j, "Info"));
      decode(v.precommit_deposit, Get(j, "PreCommitDeposit"));
      decode(v.precommit_epoch, Get(j, "PreCommitEpoch"));
      decode(v.deal_weight, Get(j, "DealWeight"));
      decode(v.verified_deal_weight, Get(j, "VerifiedDealWeight"));
    }

    ENCODE(SectorOnChainInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "SectorNumber", v.sector);
      Set(j, "SealProof", v.seal_proof);
      Set(j, "SealedCID", v.sealed_cid);
      Set(j, "DealIDs", v.deals);
      Set(j, "Activation", v.activation_epoch);
      Set(j, "Expiration", v.expiration);
      Set(j, "DealWeight", v.deal_weight);
      Set(j, "VerifiedDealWeight", v.verified_deal_weight);
      Set(j, "InitialPledge", v.init_pledge);
      Set(j, "ExpectedDayReward", v.expected_day_reward);
      Set(j, "ExpectedStoragePledge", v.expected_storage_pledge);
      return j;
    }

    DECODE(SectorOnChainInfo) {
      decode(v.sector, Get(j, "SectorNumber"));
      Get(j, "SealProof", v.seal_proof);
      decode(v.sealed_cid, Get(j, "SealedCID"));
      decode(v.deals, Get(j, "DealIDs"));
      decode(v.activation_epoch, Get(j, "Activation"));
      decode(v.expiration, Get(j, "Expiration"));
      decode(v.deal_weight, Get(j, "DealWeight"));
      decode(v.verified_deal_weight, Get(j, "VerifiedDealWeight"));
      decode(v.init_pledge, Get(j, "InitialPledge"));
      decode(v.expected_day_reward, Get(j, "ExpectedDayReward"));
      decode(v.expected_storage_pledge, Get(j, "ExpectedStoragePledge"));
    }

    ENCODE(ModularVerificationParameter) {
      Value j{rapidjson::kObjectType};
      Set(j, "Actor", v.actor);
      Set(j, "Method", v.method);
      Set(j, "Params", gsl::make_span(v.params));
      return j;
    }

    DECODE(ModularVerificationParameter) {
      decode(v.actor, Get(j, "Actor"));
      decode(v.method, Get(j, "Method"));
      decode(v.params, Get(j, "Params"));
    }

    ENCODE(Merge) {
      Value j{rapidjson::kObjectType};
      Set(j, "Lane", v.lane);
      Set(j, "Nonce", v.nonce);
      return j;
    }

    DECODE(Merge) {
      decode(v.lane, Get(j, "Lane"));
      decode(v.nonce, Get(j, "Nonce"));
    }

    ENCODE(SignedVoucher) {
      Value j{rapidjson::kObjectType};
      Set(j, "ChannelAddr", v.channel);
      Set(j, "TimeLockMin", v.time_lock_min);
      Set(j, "TimeLockMax", v.time_lock_max);
      Set(j, "SecretPreimage", gsl::make_span(v.secret_preimage));
      Set(j, "Extra", v.extra);
      Set(j, "Lane", v.lane);
      Set(j, "Nonce", v.nonce);
      Set(j, "Amount", v.amount);
      Set(j, "MinSettleHeight", v.min_close_height);
      Set(j, "Merges", v.merges);
      const auto sig{v.signature_bytes.map(
          [](auto &bytes) { return Signature::fromBytes(bytes).value(); })};
      Set(j, "Signature", sig);
      return j;
    }

    DECODE(SignedVoucher) {
      Get(j, "ChannelAddr", v.channel);
      decode(v.time_lock_min, Get(j, "TimeLockMin"));
      decode(v.time_lock_max, Get(j, "TimeLockMax"));
      decode(v.secret_preimage, Get(j, "SecretPreimage"));
      decode(v.extra, Get(j, "Extra"));
      decode(v.lane, Get(j, "Lane"));
      decode(v.nonce, Get(j, "Nonce"));
      decode(v.amount, Get(j, "Amount"));
      decode(v.min_close_height, Get(j, "MinSettleHeight"));
      decode(v.merges, Get(j, "Merges"));
      boost::optional<Signature> sig;
      Get(j, "Signature", sig);
      v.signature_bytes = sig.map([](auto &sig) { return sig.toBytes(); });
    }

    ENCODE(HeadChange) {
      const char *type{};
      if (v.type == HeadChangeType::CURRENT) {
        type = "current";
      } else if (v.type == HeadChangeType::REVERT) {
        type = "revert";
      } else if (v.type == HeadChangeType::APPLY) {
        type = "apply";
      }
      Value j{rapidjson::kObjectType};
      Set(j, "Type", type);
      Set(j, "Val", v.value);
      return j;
    }

    DECODE(HeadChange) {
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
      decode(v.value, Get(j, "Val"));
    }

    ENCODE(HealthReport) {
      Value j{rapidjson::kObjectType};
      Set(j, "Stat", v.stat);
      Set(j, "Error", v.error);
      return j;
    }

    DECODE(HealthReport) {
      decode(v.stat, Get(j, "Stat"));
      decode(v.error, Get(j, "Error"));
    }

    ENCODE(libp2p::multi::Multiaddress) {
      return encode(v.getStringAddress());
    }

    DECODE(libp2p::multi::Multiaddress) {
      OUTCOME_EXCEPT(_v, libp2p::multi::Multiaddress::create(AsString(j)));
      v = std::move(_v);
    }

    // can be generic
    template <typename T>
    ENCODE(gsl::span<T>) {
      Value j{rapidjson::kArrayType};
      j.Reserve(v.size(), allocator);
      for (const auto &elem : v) {
        j.PushBack(encode(elem), allocator);
      }
      return j;
    }

    template <>
    ENCODE(gsl::span<const UnpaddedPieceSize>) {
      Value j{rapidjson::kArrayType};
      j.Reserve(v.size(), allocator);
      for (const auto &elem : v) {
        j.PushBack(encode(elem), allocator);
      }
      return j;
    }

    ENCODE(PieceInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "Size", v.size);
      Set(j, "PieceCID", v.cid);
      return j;
    }

    DECODE(PieceInfo) {
      Get(j, "Size", v.size);
      Get(j, "PieceCID", v.cid);
    }

    ENCODE(PeerInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "ID", v.id);
      Set(j, "Addrs", v.addresses);
      return j;
    }

    DECODE(PeerInfo) {
      Get(j, "ID", v.id);
      Get(j, "Addrs", v.addresses);
    }

    ENCODE(StorageAsk) {
      Value j{rapidjson::kObjectType};
      Set(j, "Price", v.price);
      Set(j, "MinPieceSize", v.min_piece_size);
      Set(j, "Miner", v.miner);
      Set(j, "Timestamp", v.timestamp);
      Set(j, "Expiry", v.expiry);
      Set(j, "SeqNo", v.seq_no);
      return j;
    }

    DECODE(StorageAsk) {
      Get(j, "Price", v.price);
      Get(j, "MinPieceSize", v.min_piece_size);
      Get(j, "Miner", v.miner);
      Get(j, "Timestamp", v.timestamp);
      Get(j, "Expiry", v.expiry);
      Get(j, "SeqNo", v.seq_no);
    }

    ENCODE(RetrievalAsk) {
      Value j{rapidjson::kObjectType};
      Set(j, "PricePerByte", v.price_per_byte);
      Set(j, "UnsealPrice", v.unseal_price);
      Set(j, "PaymentInterval", v.payment_interval);
      Set(j, "PaymentIntervalIncrease", v.interval_increase);
      return j;
    }

    DECODE(RetrievalAsk) {
      Get(j, "PricePerByte", v.price_per_byte);
      Get(j, "UnsealPrice", v.unseal_price);
      Get(j, "PaymentInterval", v.payment_interval);
      Get(j, "PaymentIntervalIncrease", v.interval_increase);
    }

    ENCODE(AddChannelInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "Channel", v.channel);
      Set(j, "ChannelMessage", v.channel_message);
      return j;
    }

    DECODE(AddChannelInfo) {
      decode(v.channel, Get(j, "Channel"));
      decode(v.channel_message, Get(j, "ChannelMessage"));
    }

    ENCODE(SignedStorageAskV1_1_0) {
      Value j{rapidjson::kObjectType};
      Set(j, "Ask", v.ask);
      Set(j, "Signature", v.signature);
      return j;
    }

    DECODE(SignedStorageAskV1_1_0) {
      Get(j, "Ask", v.ask);
      Get(j, "Signature", v.signature);
    }

    ENCODE(DataRef) {
      Value j{rapidjson::kObjectType};
      Set(j, "TransferType", v.transfer_type);
      Set(j, "Root", v.root);
      Set(j, "PieceCid", v.piece_cid);
      Set(j, "PieceSize", v.piece_size);
      return j;
    }

    DECODE(DataRef) {
      decode(v.transfer_type, Get(j, "TransferType"));
      decode(v.root, Get(j, "Root"));
      decode(v.piece_cid, Get(j, "PieceCid"));
      v.piece_size = decode<uint64_t>(Get(j, "PieceSize"));
    }

    ENCODE(SectorExpiration) {
      Value j{rapidjson::kObjectType};
      Set(j, "OnTime", v.on_time);
      Set(j, "Early", v.early);
      return j;
    }

    DECODE(SectorExpiration) {
      decode(v.on_time, Get(j, "OnTime"));
      decode(v.early, Get(j, "Early"));
    }

    ENCODE(SectorId) {
      Value j{rapidjson::kObjectType};
      Set(j, "Miner", v.miner);
      Set(j, "Number", v.sector);
      return j;
    }

    DECODE(SectorId) {
      decode(v.miner, Get(j, "Miner"));
      decode(v.sector, Get(j, "Number"));
    }

    ENCODE(DealCollateralBounds) {
      Value j{rapidjson::kObjectType};
      Set(j, "Min", v.min);
      Set(j, "Max", v.max);
      return j;
    }

    DECODE(DealCollateralBounds) {
      Get(j, "Min", v.min);
      Get(j, "Max", v.max);
    }

    ENCODE(SectorRef) {
      Value j{rapidjson::kObjectType};
      Set(j, "ID", v.id);
      Set(j, "ProofType", v.proof_type);
      return j;
    }

    DECODE(SectorRef) {
      decode(v.id, Get(j, "ID"));
      decode(v.proof_type, Get(j, "ProofType"));
    }

    ENCODE(CallId) {
      Value j{rapidjson::kObjectType};
      Set(j, "Sector", v.sector);
      Set(j, "ID", v.id);
      return j;
    }

    DECODE(CallId) {
      decode(v.sector, Get(j, "Sector"));
      decode(v.id, Get(j, "ID"));
    }

    ENCODE(StartDealParams) {
      Value j{rapidjson::kObjectType};
      Set(j, "Data", v.data);
      Set(j, "Wallet", v.wallet);
      Set(j, "Miner", v.miner);
      Set(j, "EpochPrice", v.epoch_price);
      Set(j, "MinBlocksDuration", v.min_blocks_duration);
      Set(j, "ProviderCollateral", v.provider_collateral);
      Set(j, "DealStartEpoch", v.deal_start_epoch);
      Set(j, "FastRetrieval", v.fast_retrieval);
      Set(j, "VerifiedDeal", v.verified_deal);
      return j;
    }

    DECODE(StartDealParams) {
      decode(v.data, Get(j, "Data"));
      decode(v.wallet, Get(j, "Wallet"));
      decode(v.miner, Get(j, "Miner"));
      decode(v.epoch_price, Get(j, "EpochPrice"));
      decode(v.min_blocks_duration, Get(j, "MinBlocksDuration"));
      decode(v.provider_collateral, Get(j, "ProviderCollateral"));
      decode(v.deal_start_epoch, Get(j, "DealStartEpoch"));
      decode(v.fast_retrieval, Get(j, "FastRetrieval"));
      decode(v.verified_deal, Get(j, "VerifiedDeal"));
    }

    ENCODE(PreSealSector) {
      Value j{rapidjson::kObjectType};
      Set(j, "CommR", v.comm_r);
      Set(j, "CommD", v.comm_d);
      Set(j, "SectorID", v.sector_id);
      Set(j, "Deal", v.deal);
      Set(j, "ProofType", v.proof_type);
      return j;
    }

    DECODE(PreSealSector) {
      decode(v.comm_r, Get(j, "CommR"));
      decode(v.comm_d, Get(j, "CommD"));
      decode(v.sector_id, Get(j, "SectorID"));
      decode(v.deal, Get(j, "Deal"));
      decode(v.proof_type, Get(j, "ProofType"));
    }

    ENCODE(miner::types::Miner) {
      Value j{rapidjson::kObjectType};
      Set(j, "ID", v.id);
      Set(j, "Owner", v.owner);
      Set(j, "Worker", v.worker);
      Set(j, "PeerId", v.peer_id);
      Set(j, "MarketBalance", v.market_balance);
      Set(j, "PowerBalance", v.power_balance);
      Set(j, "SectorSize", v.sector_size);
      Set(j, "Sectors", v.sectors);
      return j;
    }

    DECODE(miner::types::Miner) {
      decode(v.id, Get(j, "ID"));
      decode(v.owner, Get(j, "Owner"));
      decode(v.worker, Get(j, "Worker"));
      decode(v.peer_id, Get(j, "PeerId"));
      decode(v.market_balance, Get(j, "MarketBalance"));
      decode(v.power_balance, Get(j, "PowerBalance"));
      decode(v.sector_size, Get(j, "SectorSize"));
      decode(v.sectors, Get(j, "Sectors"));
    }

    template <typename T>
    ENCODE(Chan<T>) {
      return encode(v.id);
    }

    template <typename T>
    DECODE(Chan<T>) {
      return decode(v.id, j);
    }

    ENCODE(Actor) {
      Value j{rapidjson::kObjectType};
      Set(j, "Code", v.code);
      Set(j, "Head", v.head);
      Set(j, "Nonce", v.nonce);
      Set(j, "Balance", v.balance);
      return j;
    }

    DECODE(Actor) {
      decode(v.code, Get(j, "Code"));
      decode(v.head, Get(j, "Head"));
      decode(v.nonce, Get(j, "Nonce"));
      decode(v.balance, Get(j, "Balance"));
    }

    ENCODE(InvocResult) {
      Value j{rapidjson::kObjectType};
      Set(j, "Msg", v.message);
      Set(j, "MsgRct", v.receipt);
      Set(j, "Error", v.error);
      return j;
    }

    DECODE(InvocResult) {
      decode(v.message, Get(j, "Msg"));
      decode(v.receipt, Get(j, "MsgRct"));
      v.error = AsString(Get(j, "Error"));
    }

    ENCODE(ExecutionResult) {
      Value j{rapidjson::kObjectType};
      Set(j, "Msg", v.message);
      Set(j, "MsgRct", v.receipt);
      Set(j, "Error", v.error);
      return j;
    }

    DECODE(ExecutionResult) {
      decode(v.message, Get(j, "Msg"));
      decode(v.receipt, Get(j, "MsgRct"));
      v.error = AsString(Get(j, "Error"));
    }

    template <typename T>
    Value encodeAs(CborDecodeStream &s) {
      T v;
      s >> v;
      return encode(v);
    }

    Value encode(CborDecodeStream &s) {
      if (s.isCid()) {
        return encodeAs<CID>(s);
      }
      if (s.isList()) {
        auto n = s.listLength();
        Value j{rapidjson::kArrayType};
        j.Reserve(n, allocator);
        auto l = s.list();
        for (; n != 0; --n) {
          j.PushBack(encode(l), allocator);
        }
        return j;
      }
      if (s.isMap()) {
        auto m = s.map();
        Value j{rapidjson::kObjectType};
        j.MemberReserve(m.size(), allocator);
        for (auto &p : m) {
          Set(j, p.first, encode(p.second));
        }
        return j;
      }
      if (s.isNull()) {
        s.next();
        return {};
      }
      if (s.isInt()) {
        return encodeAs<int64_t>(s);
      }
      if (s.isStr()) {
        return encodeAs<std::string>(s);
      }
      if (s.isBytes()) {
        return encodeAs<std::vector<uint8_t>>(s);
      }
      outcome::raise(JsonError::kWrongType);
    }

    ENCODE(IpldObject) {
      Value j{rapidjson::kObjectType};
      Set(j, "Cid", v.cid);
      CborDecodeStream s{v.raw};
      Set(j, "Obj", encode(s));
      return j;
    }

    DECODE(IpldObject) {
      outcome::raise(JsonError::kWrongType);
    }

    ENCODE(ActorState) {
      Value j{rapidjson::kObjectType};
      Set(j, "Balance", v.balance);
      Set(j, "State", v.state);
      return j;
    }

    DECODE(ActorState) {
      // Because IpldObject cannot be decoded
      outcome::raise(JsonError::kWrongType);
    }

    ENCODE(VersionResult) {
      Value j{rapidjson::kObjectType};
      Set(j, "Version", v.version);
      Set(j, "APIVersion", v.api_version);
      Set(j, "BlockDelay", v.block_delay);
      return j;
    }

    DECODE(VersionResult) {
      Get(j, "Version", v.version);
      Get(j, "APIVersion", v.api_version);
      Get(j, "BlockDelay", v.block_delay);
    }

    ENCODE(MiningBaseInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "MinerPower", v.miner_power);
      Set(j, "NetworkPower", v.network_power);
      Set(j, "Sectors", v.sectors);
      Set(j, "WorkerKey", v.worker);
      Set(j, "SectorSize", v.sector_size);
      Set(j, "PrevBeaconEntry", v.prev_beacon);
      Set(j, "BeaconEntries", v.beacons);
      Set(j, "EligibleForMining", v.has_min_power);
      return j;
    }

    DECODE(MiningBaseInfo) {
      Get(j, "MinerPower", v.miner_power);
      Get(j, "NetworkPower", v.network_power);
      Get(j, "Sectors", v.sectors);
      Get(j, "WorkerKey", v.worker);
      Get(j, "SectorSize", v.sector_size);
      Get(j, "PrevBeaconEntry", v.prev_beacon);
      Get(j, "BeaconEntries", v.beacons);
      Get(j, "EligibleForMining", v.has_min_power);
    }

    ENCODE(DealProposal) {
      Value j{rapidjson::kObjectType};
      Set(j, "PieceCID", v.piece_cid);
      Set(j, "PieceSize", v.piece_size);
      Set(j, "VerifiedDeal", v.verified);
      Set(j, "Client", v.client);
      Set(j, "Provider", v.provider);
      Set(j, "StartEpoch", v.start_epoch);
      Set(j, "EndEpoch", v.end_epoch);
      Set(j, "StoragePricePerEpoch", v.storage_price_per_epoch);
      Set(j, "ProviderCollateral", v.provider_collateral);
      Set(j, "ClientCollateral", v.client_collateral);
      return j;
    }

    DECODE(DealProposal) {
      decode(v.piece_cid, Get(j, "PieceCID"));
      v.piece_size = decode<uint64_t>(Get(j, "PieceSize"));
      Get(j, "VerifiedDeal", v.verified);
      decode(v.client, Get(j, "Client"));
      decode(v.provider, Get(j, "Provider"));
      decode(v.start_epoch, Get(j, "StartEpoch"));
      decode(v.end_epoch, Get(j, "EndEpoch"));
      decode(v.storage_price_per_epoch, Get(j, "StoragePricePerEpoch"));
      decode(v.provider_collateral, Get(j, "ProviderCollateral"));
      decode(v.client_collateral, Get(j, "ClientCollateral"));
    }

    ENCODE(DealState) {
      Value j{rapidjson::kObjectType};
      Set(j, "SectorStartEpoch", v.sector_start_epoch);
      Set(j, "LastUpdatedEpoch", v.last_updated_epoch);
      Set(j, "SlashEpoch", v.slash_epoch);
      return j;
    }

    DECODE(DealState) {
      decode(v.sector_start_epoch, Get(j, "SectorStartEpoch"));
      decode(v.last_updated_epoch, Get(j, "LastUpdatedEpoch"));
      decode(v.slash_epoch, Get(j, "SlashEpoch"));
    }

    ENCODE(StorageDeal) {
      Value j{rapidjson::kObjectType};
      Set(j, "Proposal", v.proposal);
      Set(j, "State", v.state);
      return j;
    }

    DECODE(StorageDeal) {
      decode(v.proposal, Get(j, "Proposal"));
      decode(v.state, Get(j, "State"));
    }

    ENCODE(SectorLocation) {
      Value j{rapidjson::kObjectType};
      Set(j, "Deadline", v.deadline);
      Set(j, "Partition", v.partition);
      return j;
    }

    DECODE(SectorLocation) {
      decode(v.deadline, Get(j, "Deadline"));
      decode(v.partition, Get(j, "Partition"));
    }

    ENCODE(BlockWithCids) {
      Value j{rapidjson::kObjectType};
      Set(j, "Header", v.header);
      Set(j, "BlsMessages", v.bls_messages);
      Set(j, "SecpkMessages", v.secp_messages);
      return j;
    }

    DECODE(BlockWithCids) {
      decode(v.header, Get(j, "Header"));
      decode(v.bls_messages, Get(j, "BlsMessages"));
      decode(v.secp_messages, Get(j, "SecpkMessages"));
    }

    ENCODE(QueryOffer) {
      Value j{rapidjson::kObjectType};
      Set(j, "Err", v.error);
      Set(j, "Root", v.root);
      Set(j, "Piece", v.piece);
      Set(j, "Size", v.size);
      Set(j, "MinPrice", v.min_price);
      Set(j, "UnsealPrice", v.unseal_price);
      Set(j, "PaymentInterval", v.payment_interval);
      Set(j, "PaymentIntervalIncrease", v.payment_interval_increase);
      Set(j, "Miner", v.miner);
      Set(j, "MinerPeer", v.peer);
      return j;
    }

    DECODE(QueryOffer) {
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

    ENCODE(FileRef) {
      Value j{rapidjson::kObjectType};
      Set(j, "Path", v.path);
      Set(j, "IsCAR", v.is_car);
      return j;
    }

    DECODE(FileRef) {
      decode(v.path, Get(j, "Path"));
      decode(v.is_car, Get(j, "IsCAR"));
    }

    ENCODE(ChannelId) {
      Value j{rapidjson::kObjectType};
      Set(j, "Initiator", v.initiator);
      Set(j, "Responder", v.responder);
      Set(j, "ID", v.id);
      return j;
    }

    DECODE(ChannelId) {
      decode(v.initiator, Get(j, "Initiator"));
      decode(v.responder, Get(j, "Responder"));
      decode(v.id, Get(j, "ID"));
    }

    ENCODE(DatatransferChannel) {
      Value j{rapidjson::kObjectType};
      Set(j, "TransferID", v.transfer_id);
      Set(j, "Status", v.status);
      Set(j, "BaseCID", v.base_cid);
      Set(j, "IsInitiator", v.is_initiator);
      Set(j, "IsSender", v.is_sender);
      Set(j, "Voucher", v.voucher);
      Set(j, "Message", v.message);
      Set(j, "OtherPeer", v.other_peer);
      Set(j, "Transferred", v.transferred);
      return j;
    }

    DECODE(DatatransferChannel) {
      decode(v.transfer_id, Get(j, "TransferID"));
      decode(v.status, Get(j, "Status"));
      decode(v.base_cid, Get(j, "BaseCID"));
      decode(v.is_initiator, Get(j, "IsInitiator"));
      decode(v.is_sender, Get(j, "IsSender"));
      decode(v.voucher, Get(j, "Voucher"));
      decode(v.message, Get(j, "Message"));
      decode(v.other_peer, Get(j, "OtherPeer"));
      decode(v.transferred, Get(j, "Transferred"));
    }

    ENCODE(StorageMarketDealInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "ProposalCid", v.proposal_cid);
      Set(j, "State", v.state);
      Set(j, "Message", v.message);
      Set(j, "Provider", v.provider);
      Set(j, "DataRef", v.data_ref);
      Set(j, "PieceCID", v.piece_cid);
      Set(j, "Size", v.size);
      Set(j, "PricePerEpoch", v.price_per_epoch);
      Set(j, "Duration", v.duration);
      Set(j, "DealID", v.deal_id);
      // TODO (a.chernyshov) encode and decode time type
      Set(j, "CreationTime", "2006-01-02T15:04:05Z");
      Set(j, "Verified", v.verified);
      Set(j, "TransferChannelID", v.transfer_channel_id);
      Set(j, "DataTransfer", v.data_transfer);
      return j;
    }

    DECODE(StorageMarketDealInfo) {
      decode(v.proposal_cid, Get(j, "ProposalCid"));
      decode(v.state, Get(j, "State"));
      decode(v.message, Get(j, "Message"));
      decode(v.provider, Get(j, "Provider"));
      decode(v.data_ref, Get(j, "DataRef"));
      decode(v.piece_cid, Get(j, "PieceCID"));
      decode(v.size, Get(j, "Size"));
      decode(v.price_per_epoch, Get(j, "PricePerEpoch"));
      decode(v.duration, Get(j, "Duration"));
      decode(v.deal_id, Get(j, "DealID"));
      // TODO (a.chernyshov) encode and decode time type
      v.creation_time = 0;
      decode(v.verified, Get(j, "Verified"));
      decode(v.transfer_channel_id, Get(j, "TransferChannelID"));
      decode(v.data_transfer, Get(j, "DataTransfer"));
    }

    ENCODE(ImportRes) {
      Value j{rapidjson::kObjectType};
      Set(j, "Root", v.root);
      Set(j, "ImportID", v.import_id);
      return j;
    }

    DECODE(ImportRes) {
      decode(v.root, Get(j, "Root"));
      decode(v.import_id, Get(j, "ImportID"));
    }

    ENCODE(RetrievalPeer) {
      Value j{rapidjson::kObjectType};
      Set(j, "Address", v.address);
      Set(j, "ID", v.peer_id);
      Set(j, "PieceCID", v.piece);
      return j;
    }

    DECODE(RetrievalPeer) {
      decode(v.address, Get(j, "Address"));
      decode(v.peer_id, Get(j, "ID"));
      decode(v.piece, Get(j, "PieceCID"));
    }

    ENCODE(RetrievalOrder) {
      Value j{rapidjson::kObjectType};
      Set(j, "Root", v.root);
      Set(j, "Piece", v.piece);
      Set(j, "Size", v.size);
      Set(j, "LocalStore", v.local_store);
      Set(j, "Total", v.total);
      Set(j, "UnsealPrice", v.unseal_price);
      Set(j, "PaymentInterval", v.payment_interval);
      Set(j, "PaymentIntervalIncrease", v.payment_interval_increase);
      Set(j, "Client", v.client);
      Set(j, "Miner", v.miner);
      Set(j, "MinerPeer", v.peer);
      return j;
    }

    DECODE(RetrievalOrder) {
      decode(v.root, Get(j, "Root"));
      decode(v.piece, Get(j, "Piece"));
      decode(v.size, Get(j, "Size"));
      decode(v.local_store, Get(j, "LocalStore"));
      decode(v.total, Get(j, "Total"));
      decode(v.unseal_price, Get(j, "UnsealPrice"));
      decode(v.payment_interval, Get(j, "PaymentInterval"));
      decode(v.payment_interval_increase, Get(j, "PaymentIntervalIncrease"));
      decode(v.client, Get(j, "Client"));
      decode(v.miner, Get(j, "Miner"));
      decode(v.peer, Get(j, "MinerPeer"));
    }

    ENCODE(Import) {
      Value j{rapidjson::kObjectType};
      Set(j, "StoreId", v.store_id);
      Set(j, "Error", v.error);
      Set(j, "Root", v.root);
      Set(j, "Source", v.source);
      Set(j, "FilePath", v.path);
      return j;
    }

    DECODE(Import) {
      Get(j, "StoreId", v.store_id);
      Get(j, "Error", v.error);
      Get(j, "Root", v.root);
      Get(j, "Source", v.source);
      Get(j, "FilePath", v.path);
    }

    ENCODE(LocalStorageMeta) {
      Value j{rapidjson::kObjectType};
      Set(j, "ID", v.id);
      Set(j, "Weight", v.weight);
      Set(j, "CanSeal", v.can_seal);
      Set(j, "CanStore", v.can_store);
      return j;
    }

    DECODE(LocalStorageMeta) {
      decode(v.id, Get(j, "ID"));
      decode(v.weight, Get(j, "Weight"));
      decode(v.can_seal, Get(j, "CanSeal"));
      decode(v.can_store, Get(j, "CanStore"));
    }

    ENCODE(FsStat) {
      Value j{rapidjson::kObjectType};
      Set(j, "Capacity", v.capacity);
      Set(j, "Available", v.available);
      Set(j, "Reserved", v.reserved);
      return j;
    }

    DECODE(FsStat) {
      decode(v.capacity, Get(j, "Capacity"));
      decode(v.available, Get(j, "Available"));
      decode(v.reserved, Get(j, "Reserved"));
    }

    ENCODE(WorkerInfo) {
      Value j{rapidjson::kObjectType};
      Set(j, "Hostname", v.hostname);
      Set(j, "Resources", v.resources);
      return j;
    }

    DECODE(WorkerInfo) {
      Get(j, "Hostname", v.hostname);
      Get(j, "Resources", v.resources);
    }

    ENCODE(WorkerResources) {
      Value j{rapidjson::kObjectType};
      Set(j, "MemPhysical", v.physical_memory);
      Set(j, "MemSwap", v.swap_memory);
      Set(j, "MemReserved", v.reserved_memory);
      Set(j, "CPUs", v.cpus);
      Set(j, "GPUs", v.gpus);
      return j;
    }

    DECODE(WorkerResources) {
      Get(j, "MemPhysical", v.physical_memory);
      Get(j, "MemSwap", v.swap_memory);
      Get(j, "MemReserved", v.reserved_memory);
      Get(j, "CPUs", v.cpus);
      Get(j, "GPUs", v.gpus);
    }

    template <typename T>
    ENCODE(adt::Array<T>) {
      return encode(v.amt.cid());
    }

    template <typename T>
    DECODE(adt::Array<T>) {
      v.amt = {nullptr, decode<CID>(j)};
    }

    template <typename T>
    ENCODE(boost::optional<T>) {
      if (v) {
        return encode(*v);
      }
      return {};
    }

    template <typename T>
    DECODE(boost::optional<T>) {
      if (!j.IsNull()) {
        v = decode<T>(j);
      }
    }

    ENCODE(MetaPieceData) {
      Value j{rapidjson::kObjectType};
      Set(j, "Type", v.type.toString());
      Set(j, "Info", v.uuid);
      return j;
    }

    DECODE(MetaPieceData) {
      std::string type;
      Get(j, "Type", type);
      v.type = ReaderType::fromString(type);
      Get(j, "Info", v.uuid);
    }

    template <typename T,
              typename = std::enable_if_t<!std::is_same_v<T, uint8_t>>>
    ENCODE(std::vector<T>) {
      Value j{rapidjson::kArrayType};
      j.Reserve(v.size(), allocator);
      for (auto &elem : v) {
        j.PushBack(encode(elem), allocator);
      }
      return j;
    }

    template <typename T>
    DECODE(std::vector<T>) {
      if (j.IsNull()) {
        return;
      }
      if (!j.IsArray()) {
        outcome::raise(JsonError::kWrongType);
      }
      v.reserve(j.Size());

      for (const auto &it : j.GetArray()) {
        v.emplace_back(decode<T>(it));
      }
    }

    template <typename T>
    ENCODE(std::map<std::string COMMA T>) {
      Value j{rapidjson::kObjectType};
      j.MemberReserve(v.size(), allocator);
      for (auto &pair : v) {
        Set(j, pair.first, pair.second);
      }
      return j;
    }

    template <typename T>
    DECODE(std::map<std::string COMMA T>) {
      for (auto it = j.MemberBegin(); it != j.MemberEnd(); ++it) {
        v.emplace(AsString(it->name), decode<T>(it->value));
      }
    }

    template <typename T>
    ENCODE(std::set<T>) {
      Value j{rapidjson::kArrayType};
      j.Reserve(v.size(), allocator);
      for (auto &elem : v) {
        j.PushBack(encode(elem), allocator);
      }
      return j;
    }

    template <typename T>
    DECODE(std::set<T>) {
      if (j.IsNull()) {
        return;
      }
      if (!j.IsArray()) {
        outcome::raise(JsonError::kWrongType);
      }
      for (const auto &it : j.GetArray()) {
        v.emplace(decode<T>(it));
      }
    }

    template <size_t i = 0, typename T>
    void encodeTuple(Value &j, const T &tuple) {
      if constexpr (i < std::tuple_size_v<T>) {
        j.PushBack(encode(std::get<i>(tuple)), allocator);
        encodeTuple<i + 1>(j, tuple);
      }
    }

    template <typename... T>
    ENCODE(std::tuple<T...>) {
      Value j{rapidjson::kArrayType};
      j.Reserve(sizeof...(T), allocator);
      encodeTuple(j, v);
      return j;
    }

    template <size_t i = 0, typename... T>
    DECODE(std::tuple<T...>) {
      if (!j.IsArray()) {
        outcome::raise(JsonError::kWrongType);
      }
      if constexpr (i == 0) {
        if (j.Size() > sizeof...(T)) {
          outcome::raise(JsonError::kWrongLength);
        }
      }
      if constexpr (i < sizeof...(T)) {
        if (i >= j.Size()) {
          outcome::raise(JsonError::kOutOfRange);
        }
        decode(std::get<i>(v), j[i]);
        decode<i + 1>(v, j);
      }
    }

    template <typename T>
    static T decode(const Value &j) {
      T v{codec::cbor::kDefaultT<T>()};
      decode(v, j);
      return v;
    }
  };

  template <typename T>
  Document encode(const T &v) {
    Document document;
    static_cast<Value &>(document) = Codec{document.GetAllocator()}.encode(v);
    return document;
  }

  template <typename T>
  outcome::result<T> decode(const Value &j) {
    if constexpr (std::is_void_v<T>) {
      return outcome::success();
    } else {
      try {
        return Codec::decode<T>(j);
      } catch (std::system_error &e) {
        return outcome::failure(e.code());
      }
    }
  }
}  // namespace fc::api
