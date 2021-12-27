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

  void copyStr(char *dest, const char *src) {
    if (src == nullptr) {
      return;
    }

    delete[] dest;

    dest = new char[std::strlen(src) + 1];
    std::strcpy(dest, src);
  }

  void copyStr(wchar_t *dest, const wchar_t *src) {
    if (src == nullptr) {
      return;
    }

    delete[] dest;

    dest = new wchar_t[std::wcslen(src) + 1];
    std::wcscpy(dest, src);
  }

  std::string convertToString(const std::wstring &wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(wstr);
  }

  std::string convertToString(const wchar_t *wstr_p) {
    std::wstring wstr(wstr_p);
    return convertToString(wstr);
  }

  void put2bytes(Bytes &bytes, uint16_t value) {
    bytes.push_back(static_cast<Byte>(value));
    bytes.push_back(static_cast<Byte>(value >> 8));
  }

  uint16_t getFromBytes(Byte byte1, Byte byte2) {
    return uint16_t(byte1) | uint16_t(byte2) << 8;
  }

}  // namespace ledger
