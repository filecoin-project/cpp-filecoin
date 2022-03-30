/**
* Copyright Soramitsu Co., Ltd. All Rights Reserved.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <rapidjson/document.h>

#include <array>
#include <boost/optional.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <gsl/span>
#include <map>
#include <set>
#include <tuple>
#include <vector>

#include "codec/json/json_errors.hpp"
#include "common/outcome.hpp"

#define COMMA ,

#define JSON_ENCODE(type)               \
  inline fc::codec::json::Value encode( \
      const type &v, rapidjson::MemoryPoolAllocator<> &allocator)

// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define JSON_DECODE(type) \
  inline void decode(type &v, const fc::codec::json::Value &j)

namespace fc::codec::json {
  using rapidjson::Document;
  using rapidjson::Value;
  using base64 = cppcodec::base64_rfc4648;

  template <typename T>
  T innerDecode(const Value &j);

  template <typename T>
  void Set(Value &j,
           std::string_view key,
           const T &v,
           rapidjson::MemoryPoolAllocator<> &allocator);

  template <typename T>
  inline T kDefaultT() {
    return {};
  }

  inline std::string AsString(const Value &j) {
    if (!j.IsString()) {
      outcome::raise(JsonError::kWrongType);
    }
    return {j.GetString(), j.GetStringLength()};
  }

  inline std::vector<uint8_t> decodeBase64(const Value &j) {
    if (j.IsNull()) {
      return {};
    }
    return base64::decode(AsString(j));
  }

  JSON_ENCODE(int64_t) {
    return Value{v};
  }

  JSON_DECODE(int64_t) {
    if (j.IsInt64()) {
      v = j.GetInt64();
    } else if (j.IsString()) {
      v = strtoll(j.GetString(), nullptr, 10);
    } else {
      outcome::raise(JsonError::kWrongType);
    }
  }

  JSON_ENCODE(uint64_t) {
    return Value{v};
  }

  JSON_DECODE(uint64_t) {
    if (j.IsUint64()) {
      v = j.GetUint64();
    } else if (j.IsString()) {
      v = strtoull(j.GetString(), nullptr, 10);
    } else {
      outcome::raise(JsonError::kWrongType);
    }
  }

  JSON_ENCODE(uint32_t) {
    return Value{v};
  }

  JSON_DECODE(uint32_t) {
    if (j.IsUint()) {
      v = j.GetUint();
    } else if (j.IsString()) {
      v = strtoull(j.GetString(), nullptr, 10);
    } else {
      outcome::raise(JsonError::kWrongType);
    }
  }

  JSON_ENCODE(double) {
    return Value{v};
  }

  JSON_DECODE(double) {
    if (j.IsDouble()) {
      v = j.GetDouble();
    } else if (j.IsString()) {
      v = strtod(j.GetString(), nullptr);
    } else {
      outcome::raise(JsonError::kWrongType);
    }
  }

  JSON_ENCODE(bool) {
    return Value{v};
  }

  JSON_DECODE(bool) {
    if (!j.IsBool()) {
      outcome::raise(JsonError::kWrongType);
    }
    v = j.GetBool();
  }

  JSON_ENCODE(std::string_view) {
    return {v.data(), static_cast<rapidjson::SizeType>(v.size()), allocator};
  }

  JSON_DECODE(std::string) {
    v = AsString(j);
  }

  JSON_ENCODE(gsl::span<const uint8_t>) {
    return encode(base64::encode(v.data(), v.size()), allocator);
  }

  template <size_t N>
  JSON_DECODE(std::array<uint8_t COMMA N>) {
    auto bytes = decodeBase64(j);
    if (bytes.size() != N) {
      outcome::raise(JsonError::kWrongLength);
    }
    std::copy(bytes.begin(), bytes.end(), v.begin());
  }

  JSON_DECODE(std::vector<uint8_t>) {
    v = decodeBase64(j);
  }

  template <typename T,
            typename = std::enable_if_t<!std::is_same_v<T, uint8_t>>>
  JSON_ENCODE(std::vector<T>) {
    Value j{rapidjson::kArrayType};
    j.Reserve(v.size(), allocator);
    for (const auto &elem : v) {
      j.PushBack(encode(elem, allocator), allocator);
    }
    return j;
  }

  template <typename T>
  JSON_DECODE(std::vector<T>) {
    if (j.IsNull()) {
      return;
    }
    if (!j.IsArray()) {
      outcome::raise(JsonError::kWrongType);
    }
    v.reserve(j.Size());

    for (const auto &it : j.GetArray()) {
      v.emplace_back(innerDecode<T>(it));
    }
  }

  template <typename T>
  JSON_ENCODE(std::map<std::string COMMA T>) {
    Value j{rapidjson::kObjectType};
    j.MemberReserve(v.size(), allocator);
    for (const auto &pair : v) {
      Set(j, pair.first, pair.second, allocator);
    }
    return j;
  }

  template <typename T>
  JSON_DECODE(std::map<std::string COMMA T>) {
    for (auto it = j.MemberBegin(); it != j.MemberEnd(); ++it) {
      v.emplace(AsString(it->name), innerDecode<T>(it->value));
    }
  }

  template <typename T>
  JSON_ENCODE(boost::optional<T>) {
    if (v) {
      return encode(*v, allocator);
    }
    return {};
  }

  template <typename T>
  JSON_DECODE(boost::optional<T>) {
    if (!j.IsNull()) {
      v = innerDecode<T>(j);
    }
  }

  template <size_t i = 0, typename T>
  void encodeTuple(Value &j,
                   const T &tuple,
                   rapidjson::MemoryPoolAllocator<> &allocator) {
    if constexpr (i < std::tuple_size_v<T>) {
      j.PushBack(encode(std::get<i>(tuple), allocator), allocator);
      encodeTuple<i + 1>(j, tuple, allocator);
    }
  }

  template <typename... T>
  JSON_ENCODE(std::tuple<T...>) {
    Value j{rapidjson::kArrayType};
    j.Reserve(sizeof...(T), allocator);
    encodeTuple(j, v, allocator);
    return j;
  }

  template <size_t i = 0, typename... T>
  JSON_DECODE(std::tuple<T...>) {
    if (!j.IsArray()) {
      outcome::raise(JsonError::kWrongType);
    }
    if constexpr (i == 0) {
      if (j.Size() > sizeof...(T)) {
        outcome::raise(JsonError::kWrongLength);
      }
    }
    if constexpr (i < sizeof...(T)) {
      if (i >= j.Size()) {
        outcome::raise(JsonError::kOutOfRange);
      }
      decode(std::get<i>(v), j[i]);
      decode<i + 1>(v, j);
    }
  }

  template <typename T>
  JSON_ENCODE(std::set<T>) {
    Value j{rapidjson::kArrayType};
    j.Reserve(v.size(), allocator);
    for (auto &elem : v) {
      j.PushBack(encode(elem, allocator), allocator);
    }
    return j;
  }

  template <typename T>
  JSON_DECODE(std::set<T>) {
    if (j.IsNull()) {
      return;
    }
    if (!j.IsArray()) {
      outcome::raise(JsonError::kWrongType);
    }
    for (const auto &it : j.GetArray()) {
      v.emplace(innerDecode<T>(it));
    }
  }
}  // namespace fc::codec::json
