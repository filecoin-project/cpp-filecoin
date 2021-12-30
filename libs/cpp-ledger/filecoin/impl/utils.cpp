/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/filecoin/impl/utils.hpp"

#include <cmath>

namespace ledger::filecoin {

  constexpr size_t kBip44PathLength = 5;
  constexpr size_t kUserMessageChunkSize = 250;

  void put4bytesReverse(Bytes &bytes, uint32_t value) {
    bytes.push_back(static_cast<Byte>(value));
    bytes.push_back(static_cast<Byte>(value >> 8));
    bytes.push_back(static_cast<Byte>(value >> 16));
    bytes.push_back(static_cast<Byte>(value >> 24));
  }

  std::tuple<Bytes, Error> getBip44bytes(const std::vector<uint32_t> &bip44path,
                                         size_t hardenCount) {
    if (bip44path.size() != kBip44PathLength) {
      return std::make_tuple(
          Bytes{},
          Error{"path should contain " + std::to_string(kBip44PathLength)
                + " elements"});
    }

    Bytes message;
    message.resize(sizeof(uint32_t) * kBip44PathLength);

    for (size_t i = 0; i < bip44path.size(); i++) {
      const uint32_t value =
          (i < hardenCount) ? 0x80000000 | bip44path[i] : bip44path[i];
      put4bytesReverse(message, value);
    }

    return std::make_tuple(message, Error{});
  }

  Chunks prepareChunks(const Bytes &bip44path, const Bytes &transaction) {
    const size_t packetCount =
        transaction.empty()
            ? 1
            : 2 + ((transaction.size() - 1) / kUserMessageChunkSize);

    Chunks chunks;
    chunks.reserve(packetCount);

    chunks.push_back(bip44path);

    for (size_t packetId = 1; packetId < packetCount; packetId++) {
      const size_t start = (packetId - 1) * kUserMessageChunkSize;
      const size_t end = (packetId * kUserMessageChunkSize) - 1;

      chunks.push_back(
          end >= transaction.size()
              ? Bytes(transaction.begin() + start, transaction.end())
              : Bytes(transaction.begin() + start, transaction.begin() + end));
    }

    return chunks;
  }

}  // namespace ledger::filecoin
