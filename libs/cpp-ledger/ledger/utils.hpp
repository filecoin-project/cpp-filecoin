/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cpp-ledger/common/types.hpp"

namespace ledger {

  std::string convertToString(const std::wstring &wstr);

  void put2bytes(Bytes &bytes, uint16_t value);

  uint16_t getFromBytes(Byte byte1, Byte byte2);

}  // namespace ledger
