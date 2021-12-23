/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cpp-ledger/common/types.hpp"

namespace ledger {

  void copyStr(char *dest, const char *src);

  void copyStr(wchar_t *dest, const wchar_t *src);

  std::string convertToString(const std::wstring &wstr);

  std::string convertToString(const wchar_t *wstr_p);

  void put2bytes(Bytes &bytes, uint16_t value);

  uint16_t getFromBytes(Byte byte1, Byte byte2);

}  // namespace ledger
