/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

// Copyright © 2017-2019 Dmitriy Khaustov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Dmitriy Khaustov aka xDimon
// Contacts: khaustov.dm@gmail.com
// File created on: 2017.05.30

// PercentEncoding.cpp

#include "common/uri_parser/percent_encoding.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

// unreserved
static const std::string unreserved(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "-_.~");

// reserved
static const std::string reserved("!*'();:@&=+$,/?#[]");

inline static bool need_encode(const char c) {
  if (unreserved.find(c) != std::string::npos) {
    return false;
  }
  if (reserved.find(c) != std::string::npos) {
    return true;
  }
  if (c == '%') {
    return true;
  }
  return true;
}

static int decodePercentEscapedByte(std::istringstream &iss) {
  auto c = iss.get();
  if (c != '%') {
    throw std::runtime_error("Wrong token for start percent-encoded sequence");
  }

  int result = 0;

  if (iss.eof()) {
    throw std::runtime_error(
        "Unxpected end of data during try parse percent-encoded symbol");
  }
  c = iss.get();
  if (c >= '0' && c <= '9') {
    result = c - '0';
  } else if (c >= 'a' && c <= 'f') {
    result = 10 + c - 'a';
  } else if (c >= 'A' && c <= 'F') {
    result = 10 + c - 'A';
  } else if (c == -1) {
    throw std::runtime_error(
        "Unxpected end of data during try parse percent-encoded symbol");
  } else {
    throw std::runtime_error("Wrong percent-encoded symbol");
  }

  if (iss.eof()) {
    throw std::runtime_error(
        "Unxpected end of data during try parse percent-encoded symbol");
  }
  c = iss.get();
  if (c >= '0' && c <= '9') {
    result = (result << 4) | (c - '0');
  } else if (c >= 'a' && c <= 'f') {
    result = (result << 4) | (10 + c - 'a');
  } else if (c >= 'A' && c <= 'F') {
    result = (result << 4) | (10 + c - 'A');
  } else if (c == -1) {
    throw std::runtime_error(
        "Unxpected end of data during try parse percent-encoded symbol");
  } else {
    throw std::runtime_error("Wrong percent-encoded symbol");
  }

  return result;
}

static uint32_t decodePercentEscaped(std::istringstream &iss, bool &isUtf8) {
  auto c = decodePercentEscapedByte(iss);

  if (!isUtf8) {
    return static_cast<uint32_t>(c);
  }

  int bytes;
  uint32_t symbol = 0;
  if ((c & 0b11111100) == 0b11111100) {
    bytes = 6;
    symbol = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b1);
  } else if ((c & 0b11111000) == 0b11111000) {
    bytes = 5;
    symbol = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b11);
  } else if ((c & 0b11110000) == 0b11110000) {
    bytes = 4;
    symbol = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b111);
  } else if ((c & 0b11100000) == 0b11100000) {
    bytes = 3;
    symbol = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b1111);
  } else if ((c & 0b11000000) == 0b11000000) {
    bytes = 2;
    symbol = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b11111);
  } else if ((c & 0b10000000) == 0b00000000) {
    return static_cast<uint8_t>(c) & static_cast<uint8_t>(0b1111111);
  } else {
    isUtf8 = false;
    return static_cast<uint32_t>(c);
  }

  auto fc = c;
  auto p = iss.tellg();

  while (--bytes > 0) {
    if (iss.eof()) {
      isUtf8 = false;
      return static_cast<uint32_t>(fc);
    }
    try {
      c = decodePercentEscapedByte(iss);
    } catch (...) {
      iss.clear(iss.goodbit);
      iss.seekg(p);
      return static_cast<uint32_t>(fc);
    }
    if ((c & 0b11000000) != 0b10000000) {
      iss.clear(iss.goodbit);
      iss.seekg(p);
      return static_cast<uint32_t>(fc);
    }
    symbol = (symbol << 6) | (c & 0b0011'1111);
  }

  return symbol;
}

std::string PercentEncoding::decode(const std::string &input) {
  std::istringstream iss(input);
  std::ostringstream oss;
  bool isUtf8 = true;

  while (!iss.eof()) {
    int c = iss.peek();
    if (c == '%') {
      auto symbol = decodePercentEscaped(iss, isUtf8);

      if (symbol <= 0b0111'1111)  // 7bit -> 1byte
      {
        oss.put(static_cast<uint8_t>(symbol));
      } else if (symbol <= 0b0111'1111'1111)  // 11bit -> 2byte
      {
        oss.put(
            static_cast<uint8_t>(0b1100'0000 | (0b0001'1111 & (symbol >> 6))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 0))));
      } else if (symbol <= 0b1111'1111'1111'1111)  // 16bit -> 3byte
      {
        oss.put(
            static_cast<uint8_t>(0b1110'0000 | (0b0000'1111 & (symbol >> 12))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 6))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 0))));
      } else if (symbol <= 0b0001'1111'1111'1111'1111'1111)  // 21bit -> 4byte
      {
        oss.put(
            static_cast<uint8_t>(0b1111'0000 | (0b0000'0111 & (symbol >> 18))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 12))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 6))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 0))));
      } else if (symbol
                 <= 0b0011'1111'1111'1111'1111'1111'1111)  // 26bit -> 5byte
      {
        oss.put(
            static_cast<uint8_t>(0b1111'1000 | (0b0000'0011 & (symbol >> 24))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 18))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 12))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 6))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 0))));
      } else if (symbol
                 <= 0b0111'1111'1111'1111'1111'1111'1111'1111)  // 31bit ->
                                                                // 6byte
      {
        oss.put(
            static_cast<uint8_t>(0b1111'1100 | (0b0000'0001 & (symbol >> 30))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 24))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 18))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 12))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 6))));
        oss.put(
            static_cast<uint8_t>(0b1000'0000 | (0b0011'1111 & (symbol >> 0))));
      }
    } else if (c == -1) {
      break;
    } else {
      iss.ignore();
      oss.put(static_cast<uint8_t>(c));
    }
  }

  return oss.str();
}

std::string PercentEncoding::encode(const std::string &input) {
  std::istringstream iss(input);
  std::ostringstream oss;

  while (!iss.eof()) {
    auto c = iss.get();
    if (c == -1) {
      break;
    }
    if (need_encode(static_cast<uint8_t>(c))) {
      oss << '%' << std::uppercase << std::setfill('0') << std::setw(2)
          << std::hex << c;
    } else {
      oss.put(static_cast<uint8_t>(c));
    }
  }

  return oss.str();
}
