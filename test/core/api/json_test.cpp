/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/json.hpp"

#include <gtest/gtest.h>

#include "codec/json/json.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/v0/miner/types/miner_info.hpp"

using fc::api::Address;
using fc::api::BigInt;
using fc::api::BlockHeader;
using fc::api::BlsSignature;
using fc::api::Buffer;
using fc::api::RleBitset;
using fc::api::Secp256k1Signature;
using fc::api::Signature;
using fc::api::Ticket;
using fc::api::TipsetKey;

#define J32 "\"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQE=\""
#define J65                                                                    \
  "\"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEB" \
  "AQEBAQEBAQEBAQE=\""
#define J96                                                                    \
  "\"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEB" \
  "AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEB\""

const auto b32 =
    "0101010101010101010101010101010101010101010101010101010101010101"_blob32;
const auto b65 =
    "0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob65;
const auto b96 =
    "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;

auto jsonEncode(const rapidjson::Value &value) {
  return *fc::codec::json::format(&value);
}

auto jsonDecode(fc::BytesIn str) {
  return *fc::codec::json::parse(str);
}

template <typename T>
void expectJson(const T &value, std::string _expected) {
  const auto encoded = jsonEncode(fc::api::encode(value));
  EXPECT_EQ(std::string(encoded.begin(), encoded.end()), _expected);
  EXPECT_OUTCOME_TRUE(
      decoded,
      fc::api::decode<T>(jsonDecode(fc::common::span::cbytes(_expected))));
  EXPECT_EQ(decoded, value);
}

/// Following tests check json encoding and decoding

TEST(ApiJsonTest, WrongType) {
  EXPECT_OUTCOME_ERROR(fc::api::JsonError::kWrongType,
                       fc::api::decode<Ticket>(jsonDecode(
                           fc::common::span::cbytes(std::string_view{"4"}))));
}

TEST(ApiJsonTest, Misc) {
  expectJson(INT64_C(-2), "-2");
  expectJson(std::vector<uint64_t>{1, 2}, "[1,2]");
  expectJson(RleBitset{2, 1}, "[1,2]");
  expectJson(boost::optional<uint64_t>{}, "null");
  expectJson(boost::make_optional(UINT64_C(2)), "2");
  expectJson(std::map<std::string, uint64_t>{{"a", 1}}, "{\"a\":1}");
  expectJson(std::make_tuple(UINT64_C(2), INT64_C(3)), "[2,3]");
}

TEST(ApiJsonTest, CID) {
  expectJson("010001020001"_cid, "{\"/\":\"baeaacaqaae\"}");
  expectJson(
      "122059ca84fb79f2a7447b9e82c7412df58c688910cba202b7d4e9bf329ce07f931c"_cid,
      "{\"/\":\"QmUPA6yhRBJdB6XZrXE756qBzCiEq4QXHRVX5m5Rd4Jq9u\"}");
}

TEST(ApiJsonTest, Ticket) {
  expectJson(Ticket{Buffer{b96}}, "{\"VRFProof\":" J96 "}");
}

TEST(ApiJsonTest, Address) {
  expectJson(Address::makeFromId(1), "\"t01\"");
  expectJson(Address::makeActorExec({}),
             "\"t2gncvesv7no7bqckesisllfzmif4qw3hs6fyf3iy\"");
}

TEST(ApiJsonTest, Signature) {
  expectJson(Signature{BlsSignature{b96}}, "{\"Type\":2,\"Data\":" J96 "}");
  expectJson(Signature{Secp256k1Signature{b65}},
             "{\"Type\":1,\"Data\":" J65 "}");
}

TEST(ApiJsonTest, BigInt) {
  expectJson(BigInt{0}, "\"0\"");
  expectJson(BigInt{-1}, "\"-1\"");
  expectJson(BigInt{1}, "\"1\"");
}

/**
 * @given MinerInfo without PendingWorkerKey
 * @when JSON serialized
 * @then equal to lotus serialization
 */
TEST(ApiJsonTest, MinerInfoPendingWorkerKeyNotSet) {
  using fc::vm::actor::builtin::types::miner::MinerInfo;
  MinerInfo miner_info;
  miner_info.seal_proof_type = RegisteredSealProof::kStackedDrg2KiBV1;
  EXPECT_OUTCOME_TRUE(window_post_proof_type,
                      getRegisteredWindowPoStProof(miner_info.seal_proof_type));
  miner_info.window_post_proof_type = window_post_proof_type;
  miner_info.sector_size = 1;
  miner_info.window_post_partition_sectors = 1;
  expectJson(miner_info,
             "{\"Owner\":\"t00\",\"Worker\":\"t00\",\"NewWorker\":\"<empty>\","
             "\"WorkerChangeEpoch\":-1,\"ControlAddresses\":[],\"PeerId\":null,"
             "\"Multiaddrs\":[],\"SealProofType\":0,\"WindowPoStProofType\":5,"
             "\"SectorSize\":1,\"WindowPoStPartitionSectors\":1,"
             "\"ConsensusFaultElapsed\":0}");
}

/**
 * @given MinerInfo with PendingWorkerKey present
 * @when JSON serialized
 * @then equal to lotus serialization
 */
TEST(ApiJsonTest, MinerInfoPendingWorkerKeyPresent) {
  using fc::vm::actor::builtin::types::miner::MinerInfo;
  using fc::vm::actor::builtin::types::miner::WorkerKeyChange;
  MinerInfo miner_info;
  miner_info.pending_worker_key =
      WorkerKeyChange{.new_worker = Address::makeFromId(2), .effective_at = 2};
  miner_info.seal_proof_type = RegisteredSealProof::kStackedDrg2KiBV1;
  EXPECT_OUTCOME_TRUE(window_post_proof_type,
                      getRegisteredWindowPoStProof(miner_info.seal_proof_type));
  miner_info.window_post_proof_type = window_post_proof_type;
  miner_info.sector_size = 1;
  miner_info.window_post_partition_sectors = 1;
  expectJson(miner_info,
             "{\"Owner\":\"t00\",\"Worker\":\"t00\",\"NewWorker\":\"t02\","
             "\"WorkerChangeEpoch\":2,\"ControlAddresses\":[],\"PeerId\":null,"
             "\"Multiaddrs\":[],\"SealProofType\":0,\"WindowPoStProofType\":5,"
             "\"SectorSize\":1,\"WindowPoStPartitionSectors\":1,"
             "\"ConsensusFaultElapsed\":0}");
}
