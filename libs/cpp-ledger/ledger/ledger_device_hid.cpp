/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/ledger/ledger_device_hid.hpp"

#include <cmath>
#include "cpp-ledger/ledger/apdu_wrapper.hpp"
#include "cpp-ledger/ledger/const.hpp"
#include "cpp-ledger/ledger/utils.hpp"

namespace ledger {

  constexpr size_t kMinCommandSize = 5;
  constexpr size_t kMinResponseSize = 2;

  std::tuple<Bytes, Error> LedgerDeviceHid::Exchange(
      const Bytes &command) const {
    if (command.size() < kMinCommandSize) {
      return std::make_tuple(Bytes{},
                             Error{"APDU commands should not be smaller than "
                                   + std::to_string(kMinCommandSize)});
    }

    if (command.size() - kMinCommandSize != command[4]) {
      return std::make_tuple(Bytes{}, Error{"APDU[data length] mismatch"});
    }

    auto [serializedCommand, err] =
        apdu::WrapCommandApdu(kChannel, command, kPacketSize);
    if (err != std::nullopt) {
      return std::make_tuple(Bytes{}, err);
    }

    // Write all the packets
    std::tie(std::ignore, err) = Write(serializedCommand);
    if (err != std::nullopt) {
      return std::make_tuple(Bytes{}, err);
    }

    const auto readData = Read();

    const auto [response, err1] = apdu::UnwrapResponseApdu(kChannel, readData);
    if (err1 != std::nullopt) {
      return std::make_tuple(Bytes{}, err1);
    }

    if (response.size() < kMinResponseSize) {
      return std::make_tuple(
          Bytes{},
          Error{"response length < " + std::to_string(kMinResponseSize)});
    }

    const auto swOffset = response.size() - kMinResponseSize;
    const auto sw = getFromBytes(response[swOffset], response[swOffset + 1]);

    const Bytes result(response.begin(), response.begin() + swOffset);

    if (sw != 0x9000) {
      return std::make_tuple(result, apdu::ErrorMessage(sw));
    }

    return std::make_tuple(result, Error{});
  }

  void LedgerDeviceHid::Close() {
    device.Close();
  }

  std::tuple<int, Error> LedgerDeviceHid::Write(const Bytes &bytes) const {
    size_t totalWrittenBytes = 0;
    auto buffer = bytes;
    while (bytes.size() > totalWrittenBytes) {
      const auto [writtenBytes, err] = device.Write(buffer);

      if (err != std::nullopt) {
        return std::make_tuple(totalWrittenBytes, err);
      }

      buffer = Bytes(buffer.begin() + writtenBytes, buffer.end());
      totalWrittenBytes += writtenBytes;
    }
    return std::make_tuple(totalWrittenBytes, Error{});
  }

  std::vector<Bytes> LedgerDeviceHid::Read() const {
    std::vector<Bytes> result;

    size_t packet_count = 0;

    while (true) {
      Bytes buffer(kPacketSize);
      const auto [readBytes, err] = device.Read(buffer);

      if (err != std::nullopt) {
        break;
      }

      if (packet_count == 0) {
        std::tie(std::ignore, packet_count, std::ignore) =
            apdu::DeserializePacket(kChannel, buffer, 0);
        packet_count = std::ceil(packet_count / double(kPacketSize));
      }

      result.push_back(readBytes == kPacketSize
                           ? buffer
                           : Bytes(buffer.begin(), buffer.begin() + readBytes));

      if (result.size() >= packet_count) {
        // It means Ledger has finished sending the response
        break;
      }
    }

    return result;
  }

}  // namespace ledger
