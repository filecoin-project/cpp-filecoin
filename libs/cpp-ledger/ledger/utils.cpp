/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/ledger/utils.hpp"

#include <wchar.h>
#include <codecvt>
#include <cstring>
#include <locale>

namespace ledger {

  std::string convertToString(const std::wstring &wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(wstr);
  }

  void put2bytes(Bytes &bytes, uint16_t value) {
    bytes.push_back(static_cast<Byte>(value));
    bytes.push_back(static_cast<Byte>(value >> 8));
  }

  uint16_t getFromBytes(Byte byte1, Byte byte2) {
    return uint16_t(byte1) | uint16_t(byte2) << 8;
  }

}  // namespace ledger
