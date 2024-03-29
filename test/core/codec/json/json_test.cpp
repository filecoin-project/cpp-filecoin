/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

#define J32 "\"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQE=\""
#define J65                                                                    \
  "\"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEB" \
  "AQEBAQEBAQEBAQE=\""
#define J96                                                                    \
  "\"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEB" \
  "AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEB\""

namespace fc::codec::json {
  using api::MinerInfo;
  using crypto::signature::BlsSignature;
  using crypto::signature::Secp256k1Signature;
  using crypto::signature::Signature;
  using primitives::RleBitset;
  using primitives::address::Address;
  using primitives::block::Ticket;
  using primitives::sector::RegisteredPoStProof;

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
    auto encoded = jsonEncode(encode<T>(value));
    EXPECT_EQ(std::string(encoded.begin(), encoded.end()), _expected);
    EXPECT_OUTCOME_TRUE(
        decoded, decode<T>(jsonDecode(fc::common::span::cbytes(_expected))));
    encoded = jsonEncode(encode<T>(decoded));
    EXPECT_EQ(std::string(encoded.begin(), encoded.end()), _expected);
  }

  /// Following tests check json encoding and decoding

  TEST(ApiJsonTest, WrongType) {
    EXPECT_OUTCOME_ERROR(JsonError::kWrongType,
                         decode<Ticket>(jsonDecode(
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
    expectJson(Ticket{copy(b96)}, "{\"VRFProof\":" J96 "}");
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
    MinerInfo miner_info;
    miner_info.window_post_proof_type =
        RegisteredPoStProof::kStackedDRG2KiBWindowPoSt;
    miner_info.sector_size = 1;
    miner_info.window_post_partition_sectors = 1;
    expectJson(miner_info,
               "{\"Owner\":\"t00\",\"Worker\":\"t00\",\"ControlAddresses\":[],"
               "\"PeerId\":null,"
               "\"Multiaddrs\":[],\"WindowPoStProofType\":5,"
               "\"SectorSize\":1,\"WindowPoStPartitionSectors\":1}");
  }

  /**
   * @given std::set of strings
   * @when JSON serialization
   * @then encoded as an array of strings
   */
  TEST(ApiJsonTest, SetOfStringEncoding) {
    std::set<std::string> s;
    s.emplace("a");
    s.emplace("b");
    s.emplace("c");
    expectJson(s, R"(["a","b","c"])");
  }

  /**
   * @given std::set of strings as CodecSetAsMap
   * @when JSON serialization
   * @then encoded as a map of string
   */
  TEST(ApiJsonTest, SetOfStringEncodingAsMap) {
    std::set<std::string> s;
    s.emplace("a");
    s.emplace("b");
    s.emplace("c");
    expectJson(api::CodecSetAsMap<std::string>{s},
               R"({"a":{},"b":{},"c":{}})");
  }
}  // namespace fc::api
