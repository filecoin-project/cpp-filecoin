/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/json/basic_coding.hpp"

namespace fc::codec::json {

  template <typename T>
  inline void decodeEnum(T &v, const Value &j) {
    v = T{innerDecode<std::underlying_type_t<T>>(j)};
  }

  inline void Set(Value &j,
                  std::string_view key,
                  Value &&value,
                  rapidjson::MemoryPoolAllocator<> &allocator) {
    j.AddMember(encode(key, allocator), value, allocator);
  }

  template <typename T>
  inline void Set(Value &j,
                  std::string_view key,
                  const T &v,
                  rapidjson::MemoryPoolAllocator<> &allocator) {
    Set(j, key, encode(v, allocator), allocator);
  }

  inline const Value &Get(const Value &j, const char *key) {
    if (!j.IsObject()) {
      outcome::raise(JsonError::kWrongType);
    }
    auto it = j.FindMember(key);
    if (it == j.MemberEnd()) {
      outcome::raise(JsonError::kOutOfRange);
    }
    return it->value;
  }

  template <typename T>
  inline void Get(const Value &j, const char *key, T &v) {
    decode(v, Get(j, key));
  }

  template <typename T>
  inline Document encode(const T &v) {
    Document document;
    static_cast<Value &>(document) = encode(v, document.GetAllocator());
    return document;
  }

  template <typename T>
  inline T innerDecode(const Value &j) {
    T v{kDefaultT<T>()};
    decode(v, j);
    return v;
  }

  template <typename T>
  inline outcome::result<T> decode(const Value &j) {
    if constexpr (std::is_void_v<T>) {
      return outcome::success();
    } else {
      try {
        return innerDecode<T>(j);
      } catch (std::system_error &e) {
        return outcome::failure(e.code());
      }
    }
  }
}  // namespace fc::codec::json
