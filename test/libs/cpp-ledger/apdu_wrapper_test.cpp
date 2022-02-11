/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/ledger/apdu_wrapper.hpp"

#include <gtest/gtest.h>
#include "cpp-ledger/ledger/utils.hpp"

namespace ledger::apdu {

  TEST(ApduWrapperTest, SerializePacket) {
    constexpr int packetSize = 64;
    constexpr uint16_t channel = 0x0101;
    constexpr uint16_t sequenceId = 0;
    constexpr uint16_t commandLen = 100;

    const Bytes command(commandLen, 0);

    const auto [result, offset, err] =
        SerializePacket(channel, command, packetSize, sequenceId);

    EXPECT_EQ(result.size(), packetSize);
    EXPECT_EQ(getFromBytes(result[0], result[1]), channel);
    EXPECT_EQ(result[2], kTag);
    EXPECT_EQ(getFromBytes(result[3], result[4]), sequenceId);
    EXPECT_EQ(getFromBytes(result[5], result[6]), commandLen);

    EXPECT_EQ(offset,
              packetSize
                  - (sizeof(channel) + sizeof(kTag) + sizeof(sequenceId)
                     + sizeof(commandLen)));

    EXPECT_EQ(err, std::nullopt);
  }

  TEST(ApduWrapperTest, DeserializeFirstPacket) {
    const Bytes command{'H', 'e', 'l', 'l', 'o', 0};

    constexpr int packetSize = 64;
    constexpr uint16_t channel = 0x0101;
    constexpr uint16_t sequenceId = 0;
    const uint16_t commandLen = command.size();

    constexpr size_t packetHeaderSize = sizeof(channel) + sizeof(kTag)
                                        + sizeof(sequenceId)
                                        + sizeof(commandLen);

    const auto [packet, offset, err1] =
        SerializePacket(channel, command, packetSize, sequenceId);

    const auto [output, totalSize, err2] =
        DeserializePacket(channel, packet, sequenceId);

    EXPECT_EQ(err2, std::nullopt);
    EXPECT_EQ(totalSize, commandLen);
    EXPECT_EQ(output.size(), packetSize - packetHeaderSize);
    EXPECT_EQ(Bytes(output.begin(), output.begin() + commandLen), command);
  }

  TEST(ApduWrapperTest, DeserializeSecondPacket) {
    const Bytes command{'H', 'e', 'l', 'l', 'o', 0};

    constexpr int packetSize = 64;
    constexpr uint16_t channel = 0x0101;
    constexpr uint16_t sequenceId = 1;

    constexpr size_t packetHeaderSize =
        sizeof(channel) + sizeof(kTag) + sizeof(sequenceId);

    const auto [packet, offset, err1] =
        SerializePacket(channel, command, packetSize, sequenceId);

    const auto [output, totalSize, err2] =
        DeserializePacket(channel, packet, sequenceId);

    EXPECT_EQ(err2, std::nullopt);
    EXPECT_EQ(totalSize, 0);
    EXPECT_EQ(output.size(), packetSize - packetHeaderSize);
    EXPECT_EQ(Bytes(output.begin(), output.begin() + command.size()), command);
  }

  TEST(ApduWrapperTest, WrapCommandApdu) {
    constexpr int packetSize = 64;
    constexpr uint16_t channel = 0x0101;
    constexpr uint16_t startSequenceId = 0;
    constexpr uint16_t commandLen = 200;

    constexpr size_t packetCount = 4;
    constexpr size_t firstHeaderSize = sizeof(channel) + sizeof(kTag)
                                       + sizeof(startSequenceId)
                                       + sizeof(commandLen);
    constexpr size_t nonFirstHeaderSize =
        sizeof(channel) + sizeof(kTag) + sizeof(startSequenceId);

    Bytes command;
    command.reserve(commandLen);

    for (Byte i = 0; i < commandLen; i++) {
      command.push_back(i % 256);
    }

    const auto [result, err] = WrapCommandApdu(channel, command, packetSize);

    EXPECT_EQ(result.size(), packetCount * packetSize);

    size_t commandStart = 0;

    for (auto i = startSequenceId; i < packetCount; i++) {
      // check header
      EXPECT_EQ(
          getFromBytes(result[packetSize * i + 0], result[packetSize * i + 1]),
          channel);
      EXPECT_EQ(result[packetSize * i + 2], kTag);
      EXPECT_EQ(
          getFromBytes(result[packetSize * i + 3], result[packetSize * i + 4]),
          i);

      if (i == 0) {
        EXPECT_EQ(getFromBytes(result[5], result[6]), commandLen);
      }

      const auto headerSize = i == 0 ? firstHeaderSize : nonFirstHeaderSize;
      const size_t commandSize = packetSize - headerSize;

      // check data
      const Bytes res(result.begin() + packetSize * i + headerSize,
                      result.begin() + packetSize * (i + 1));
      Bytes cmd = (command.size() > commandStart + commandSize)
                      ? Bytes(command.begin() + commandStart,
                              command.begin() + commandStart + commandSize)
                      : Bytes(command.begin() + commandStart, command.end());
      cmd.resize(commandSize, 0);

      EXPECT_EQ(res, cmd);

      commandStart += commandSize;
    }

    EXPECT_EQ(err, std::nullopt);
  }

  TEST(ApduWrapperTest, UnwrapResponseApdu) {
    constexpr int packetSize = 64;
    constexpr uint16_t channel = 0x8002;
    constexpr uint16_t inputSize = 200;

    Bytes input;
    input.reserve(inputSize);

    for (Byte i = 0; i < inputSize; i++) {
      input.push_back(i % 256);
    }

    const auto [serialized, err1] = WrapCommandApdu(channel, input, packetSize);

    std::vector<Bytes> packets;

    for (size_t i = 0; i < serialized.size(); i += packetSize) {
      packets.push_back(
          Bytes(serialized.begin() + i, serialized.begin() + i + packetSize));
    }

    const auto [output, err2] = UnwrapResponseApdu(channel, packets);

    EXPECT_EQ(err2, std::nullopt);
    EXPECT_EQ(output, input);
  }

}  // namespace ledger::apdu
