/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/ledger/ledger_device_hid.hpp"

#include "cpp-ledger/ledger/apdu_wrapper.hpp"
#include "cpp-ledger/ledger/const.hpp"
#include "cpp-ledger/ledger/utils.hpp"

namespace ledger {

  std::tuple<Bytes, Error> LedgerDeviceHid::Exchange(
      const Bytes &command) const {
    if (command.size() < 5) {
      return std::make_tuple(
          Bytes{}, Error{"APDU commands should not be smaller than 5"});
    }

    if (command.size() - 5 != command[4]) {
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

    const auto [response, err1] =
        apdu::UnwrapResponseApdu(kChannel, readData, kPacketSize);
    if (err1 != std::nullopt) {
      return std::make_tuple(Bytes{}, err1);
    }

    if (response.size() < 2) {
      return std::make_tuple(Bytes{}, Error{"response length < 2"});
    }

    const auto swOffset = response.size() - 2;
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

    while (true) {
      Bytes buffer(kPacketSize);
      const auto [readBytes, err] = device.Read(buffer);

      if (err != std::nullopt) {
        break;
      }

      result.push_back(readBytes == kPacketSize
                           ? buffer
                           : Bytes(buffer.begin(), buffer.begin() + readBytes));
    }

    return result;
  }

}  // namespace ledger
