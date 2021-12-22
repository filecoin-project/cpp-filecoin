/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <wchar.h>
#include <codecvt>
#include <cstdint>
#include <locale>
#include <optional>
#include <string>
#include <vector>

namespace ledger {
  using Byte = uint8_t;
  using Bytes = std::vector<Byte>;
  using Error = std::optional<std::string>;

  constexpr uint16_t kVendorLedger = 0x2c97;
  constexpr uint16_t kUsagePageLedgerNanoS = 0xffa0;
  constexpr uint16_t kChannel = 0x0101;
  constexpr int kPacketSize = 64;

  void copyStr(char *dest, const char *src) {
    if (src == nullptr) {
      return;
    }

    if (dest != nullptr) {
      delete[] dest;
    }

    dest = new char[std::strlen(src) + 1];
    std::strcpy(dest, src);
  }

  void copyStr(wchar_t *dest, const wchar_t *src) {
    if (dest != nullptr) {
      delete[] dest;
    }

    dest = new wchar_t[std::wcslen(src) + 1];
    std::wcscpy(dest, src);
  }

  std::string convertToString(const std::wstring &wsrt) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(wsrt);
  }

  void put2bytes(Bytes &bytes, uint16_t value) {
    bytes.push_back(static_cast<Byte>(value));
    bytes.push_back(static_cast<Byte>(value >> 8));
  }

  uint16_t getFromBytes(Byte byte1, Byte byte2) {
    return uint16_t(byte1) | uint16_t(byte2) << 8;
  }

}  // namespace ledger
