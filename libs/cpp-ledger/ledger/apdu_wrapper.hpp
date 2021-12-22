/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common.hpp"

namespace ledger::apdu {

  Error ErrorMessage(uint16_t errCode);

  std::tuple<Bytes, int, Error> SerializePacket(uint16_t channel,
                                                const Bytes &command,
                                                int packetSize,
                                                uint16_t sequenceId);

  std::tuple<Bytes, uint16_t, Error> DeserializePacket(uint16_t channel,
                                                       const Bytes &buffer,
                                                       uint16_t sequenceId);

  // WrapCommandAPDU turns the command into a sequence of 64 byte packets
  std::tuple<Bytes, Error> WrapCommandApdu(uint16_t channel,
                                           Bytes command,
                                           int packetSize);

  // UnwrapResponseAPDU parses a response of 64 byte packets into the real data
  std::tuple<Bytes, Error> UnwrapResponseApdu(uint16_t channel,
                                              const std::vector<Bytes> &packets,
                                              int packetSize);

}  // namespace ledger::apdu
