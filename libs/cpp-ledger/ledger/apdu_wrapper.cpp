/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "apdu_wrapper.hpp"

#include "utils.hpp"

namespace ledger::apdu {

  Error ErrorMessage(uint16_t errCode) {
    switch (errCode) {
      case 0x6400:
        return Error{
            "[APDU_CODE_EXECUTION_ERROR] No information given (NV-Ram not "
            "changed)"};
      case 0x6700:
        return Error{"[APDU_CODE_WRONG_LENGTH] Wrong length"};
      case 0x6982:
        return Error{
            "[APDU_CODE_EMPTY_BUFFER] Security condition not satisfied"};
      case 0x6983:
        return Error{
            "[APDU_CODE_OUTPUT_BUFFER_TOO_SMALL] Authentication method "
            "blocked"};
      case 0x6984:
        return Error{
            "[APDU_CODE_DATA_INVALID] Referenced data reversibly blocked "
            "(invalidated)"};
      case 0x6985:
        return Error{
            "[APDU_CODE_CONDITIONS_NOT_SATISFIED] Conditions of use not "
            "satisfied"};
      case 0x6986:
        return Error{
            "[APDU_CODE_COMMAND_NOT_ALLOWED] Command not allowed (no current "
            "EF)"};
      case 0x6A80:
        return Error{
            "[APDU_CODE_BAD_KEY_HANDLE] The parameters in the data field are "
            "incorrect"};
      case 0x6B00:
        return Error{"[APDU_CODE_INVALID_P1P2] Wrong parameter(s) P1-P2"};
      case 0x6D00:
        return Error{
            "[APDU_CODE_INS_NOT_SUPPORTED] Instruction code not supported or "
            "invalid"};
      case 0x6E00:
        return Error{"[APDU_CODE_CLA_NOT_SUPPORTED] Class not supported"};
      case 0x6F00:
        return Error{"APDU_CODE_UNKNOWN"};
      case 0x6F01:
        return Error{"APDU_CODE_SIGN_VERIFY_ERROR"};
      default:
        return Error{"Error code: " + std::to_string(errCode)};
    }
  }

  std::tuple<Bytes, int, Error> SerializePacket(uint16_t channel,
                                                const Bytes &command,
                                                int packetSize,
                                                uint16_t sequenceId) {
    if (packetSize < 3) {
      return std::make_tuple(
          Bytes{}, 0, Error{"Packet size must be at least 3"});
    }

    Byte headerOffset = 0;

    Bytes result;
    result.reserve(packetSize);

    // Insert channel (2 bytes)
    put2bytes(result, channel);
    headerOffset += 2;

    // Insert tag (1 byte)
    result.push_back(0x05);
    headerOffset++;

    // Insert sequenceId (2 bytes)
    put2bytes(result, sequenceId);
    headerOffset += 2;

    // Only insert total size of the command in the first package
    if (sequenceId == 0) {
      // Insert command length (2 bytes)
      put2bytes(result, command.size());
      headerOffset += 2;
    }

    size_t offset = packetSize - headerOffset;
    if (command.size() <= offset) {
      result.insert(result.end(), command.begin(), command.end());
      offset = command.size();
    } else {
      result.insert(result.end(), command.begin(), command.begin() + offset);
      for (auto size = packetSize - result.size(); size > 0; size--) {
        result.push_back(0);
      }
    }

    return std::make_tuple(result, offset, Error{});
  }

  std::tuple<Bytes, uint16_t, Error> DeserializePacket(uint16_t channel,
                                                       const Bytes &buffer,
                                                       uint16_t sequenceId) {
    if ((sequenceId == 0 && buffer.size() < 7)
        || (sequenceId > 0 && buffer.size() < 5)) {
      return std::make_tuple(
          Bytes{},
          0,
          Error{
              "Cannot deserialize the packet. Header information is missing."});
    }

    Byte headerOffset = 0;

    if (getFromBytes(buffer[0], buffer[1]) != channel) {
      return std::make_tuple(Bytes{}, 0, Error{"Invalid channel"});
    }
    headerOffset += 2;

    if (buffer[headerOffset] != 0x05) {
      return std::make_tuple(Bytes{}, 0, Error{"Invalid tag"});
    }
    headerOffset++;

    if (getFromBytes(buffer[headerOffset], buffer[headerOffset + 1])
        != channel) {
      return std::make_tuple(Bytes{}, 0, Error{"Wrong sequenceId"});
    }
    headerOffset += 2;

    uint16_t totalResponseLength = 0;

    if (sequenceId == 0) {
      totalResponseLength =
          getFromBytes(buffer[headerOffset], buffer[headerOffset + 1]);
      headerOffset += 2;
    }

    const Bytes result(buffer.begin() + headerOffset, buffer.end());

    return std::make_tuple(result, totalResponseLength, Error{});
  }

  std::tuple<Bytes, Error> WrapCommandApdu(uint16_t channel,
                                           Bytes command,
                                           int packetSize) {
    Bytes totalResult;
    uint16_t sequenceId = 0;
    while (!command.empty()) {
      const auto [result, offset, err] =
          SerializePacket(channel, command, packetSize, sequenceId);
      if (err != std::nullopt) {
        return std::make_tuple(Bytes{}, err);
      }
      command = Bytes(command.begin() + offset, command.end());
      totalResult.insert(totalResult.end(), result.begin(), result.end());
      sequenceId++;
    }

    return std::make_tuple(totalResult, Error{});
  }

  std::tuple<Bytes, Error> UnwrapResponseApdu(uint16_t channel,
                                              const std::vector<Bytes> &packets,
                                              int packetSize) {
    Bytes totalResult;
    uint16_t totalSize = 0;
    uint16_t sequenceId = 0;

    for (const auto &packet : packets) {
      const auto [result, responseSize, err] =
          DeserializePacket(kChannel, packet, sequenceId);

      if (err != std::nullopt) {
        return std::make_tuple(Bytes{}, err);
      }

      if (sequenceId == 0) {
        totalSize = responseSize;
        totalResult.reserve(totalSize);
      }

      totalResult.insert(totalResult.end(), result.begin(), result.end());
      sequenceId++;
    }

    totalResult = Bytes(totalResult.begin(), totalResult.begin() + totalSize);
    return std::make_tuple(totalResult, Error{});
  }

}  // namespace ledger::apdu
