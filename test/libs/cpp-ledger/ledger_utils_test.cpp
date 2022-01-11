/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/filecoin/impl/utils.hpp"
#include "cpp-ledger/filecoin/types/version_info.hpp"

#include <gtest/gtest.h>
#include <iomanip>
#include <sstream>

namespace ledger::filecoin {

  std::string toHexString(const Bytes &bytes) {
    std::stringstream ss;

    for (const Byte byte : bytes) {
      ss << std::setfill('0') << std::setw(2) << std::hex << int(byte);
    }

    return ss.str();
  }

  TEST(UtilsTest, PrintVersion) {
    const VersionInfo version{.appMode = 0, .major = 1, .minor = 2, .patch = 3};
    EXPECT_EQ(version.ToString(), "1.2.3");
  }

  TEST(UtilsTest, WrongPath1) {
    const std::vector<uint32_t> bip44path{44, 100, 0, 0};

    const auto [path, err] = getBip44bytes(bip44path, 0);
    EXPECT_EQ(err, "path should contain 5 elements");
  }

  TEST(UtilsTest, WrongPath2) {
    const std::vector<uint32_t> bip44path{44, 100, 0, 0, 0, 3};

    const auto [path, err] = getBip44bytes(bip44path, 0);
    EXPECT_EQ(err, "path should contain 5 elements");
  }

  TEST(UtilsTest, PathGeneration1) {
    const std::vector<uint32_t> bip44path{44, 100, 0, 0, 0};

    const auto [path, err] = getBip44bytes(bip44path, 0);

    EXPECT_EQ(err, std::nullopt);
    EXPECT_EQ(path.size(), 20);

    EXPECT_EQ(toHexString(path), "2c00000064000000000000000000000000000000");
  }

  TEST(UtilsTest, PathGeneration2) {
    const std::vector<uint32_t> bip44path{44, 123, 0, 0, 0};

    const auto [path, err] = getBip44bytes(bip44path, 2);

    EXPECT_EQ(err, std::nullopt);
    EXPECT_EQ(path.size(), 20);

    EXPECT_EQ(toHexString(path), "2c0000807b000080000000000000000000000000");
  }

  TEST(UtilsTest, PathGeneration3) {
    const std::vector<uint32_t> bip44path{44, 123, 0, 0, 0};

    const auto [path, err] = getBip44bytes(bip44path, 3);

    EXPECT_EQ(err, std::nullopt);
    EXPECT_EQ(path.size(), 20);

    EXPECT_EQ(toHexString(path), "2c0000807b000080000000800000000000000000");
  }

  TEST(UtilsTest, PrepareEmptyChunk) {
    const std::vector<uint32_t> bip44path{44, 123, 0, 0, 0};
    const auto [path, err] = getBip44bytes(bip44path, 0);
    EXPECT_EQ(err, std::nullopt);

    const auto chunks = prepareChunks(path, {});

    EXPECT_EQ(chunks.size(), 1);
    EXPECT_EQ(chunks[0], path);
  }

  TEST(UtilsTest, Prepare1Chunk) {
    const std::vector<uint32_t> bip44path{44, 123, 0, 0, 0};
    const auto [path, err] = getBip44bytes(bip44path, 0);
    EXPECT_EQ(err, std::nullopt);

    const Bytes message{0x88, 0x55, 0x01, 0xfd, 0x1d, 0x0f, 0x4d, 0xfc, 0xd7,
                        0xe9, 0x9a, 0xfc, 0xb9, 0x9a, 0x83, 0x26, 0xb7, 0xdc,
                        0x45, 0x9d, 0x32, 0xc6, 0x28, 0x55, 0x01, 0xb8, 0x82,
                        0x61, 0x9d, 0x46, 0x55, 0x8f, 0x3d, 0x9e, 0x31, 0x6d,
                        0x11, 0xb4, 0x8d, 0xcf, 0x21, 0x13, 0x27, 0x02, 0x5a,
                        0x01, 0x44, 0x00, 0x01, 0x86, 0xa0, 0x43, 0x00, 0x09,
                        0xc4, 0x43, 0x00, 0x61, 0xa8, 0x00, 0x40};

    const auto chunks = prepareChunks(path, message);

    EXPECT_EQ(chunks.size(), 2);
    EXPECT_EQ(chunks[0], path);
    EXPECT_EQ(chunks[1], message);
  }

  TEST(UtilsTest, PrepareSeveralChunks) {
    const std::vector<uint32_t> bip44path{44, 123, 0, 0, 0};
    const auto [path, err] = getBip44bytes(bip44path, 0);
    EXPECT_EQ(err, std::nullopt);

    Bytes message;
    Bytes expectedChunk1;
    Bytes expectedChunk2;
    Bytes expectedChunk3;

    for (size_t i = 0; i < 700; i++) {
      const Byte value = i % 256;

      message.push_back(value);

      if (i < 250) {
        expectedChunk1.push_back(value);
      }
      if (i >= 250 && i < 500) {
        expectedChunk2.push_back(value);
      }
      if (i >= 500 && i < 750) {
        expectedChunk3.push_back(value);
      }
    }

    const auto chunks = prepareChunks(path, message);

    EXPECT_EQ(chunks.size(), 4);
    EXPECT_EQ(chunks[0], path);
    EXPECT_EQ(chunks[1], expectedChunk1);
    EXPECT_EQ(chunks[2], expectedChunk2);
    EXPECT_EQ(chunks[3], expectedChunk3);
  }

}  // namespace ledger::filecoin
