/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_RPC_JSON_HPP
#define CPP_FILECOIN_CORE_API_RPC_JSON_HPP

#include <rapidjson/document.h>
#include <cppcodec/base64_rfc4648.hpp>

#include "api/api.hpp"
#include "api/rpc/json_errors.hpp"

#define COMMA ,

#define ENCODE(type) Value encode(const type &v)

#define DECODE(type) static void decode(type &v, const Value &j)

namespace fc::api {
  using primitives::ticket::Ticket;
  using rapidjson::Document;
  using rapidjson::Value;
  using base64 = cppcodec::base64_rfc4648;

  struct Codec {
    rapidjson::MemoryPoolAllocator<> &allocator;

    static std::string AsString(const Value &j) {
      if (!j.IsString()) {
        outcome::raise(JsonError::WRONG_TYPE);
      }
      return {j.GetString(), j.GetStringLength()};
    }

    static const Value &Get(const Value &j, const char *key) {
      if (!j.IsObject()) {
        outcome::raise(JsonError::WRONG_TYPE);
      }
      auto it = j.FindMember(key);
      if (it == j.MemberEnd()) {
        outcome::raise(JsonError::OUT_OF_RANGE);
      }
      return it->value;
    }

    template <typename T>
    void Set(Value &j, std::string_view key, const T &v) {
      j.AddMember(encode(key), encode(v), allocator);
    }

    static auto decodeBase64(const Value &j) {
      return base64::decode(AsString(j));
    }

    ENCODE(std::string_view) {
      return {v.data(), static_cast<rapidjson::SizeType>(v.size()), allocator};
    }

    ENCODE(gsl::span<const uint8_t>) {
      return encode(base64::encode(v.data(), v.size()));
    }

    template <size_t N>
    DECODE(std::array<uint8_t COMMA N>) {
      auto bytes = decodeBase64(j);
      if (bytes.size() != N) {
        outcome::raise(JsonError::WRONG_LENGTH);
      }
      std::copy(bytes.begin(), bytes.end(), v.begin());
    }

    ENCODE(CID) {
      OUTCOME_EXCEPT(str, v.toString());
      Value j{rapidjson::kObjectType};
      Set(j, "/", str);
      return j;
    }

    DECODE(CID) {
      OUTCOME_EXCEPT(cid, CID::fromString(AsString(Get(j, "/"))));
      v = std::move(cid);
    }

    ENCODE(Ticket) {
      Value j{rapidjson::kObjectType};
      Set(j, "VRFProof", gsl::make_span(v.bytes));
      return j;
    }

    DECODE(Ticket) {
      decode(v.bytes, Get(j, "VRFProof"));
    }

    template <typename T>
    static T decode(const Value &j) {
      T v;
      decode(v, j);
      return std::move(v);
    }
  };

  template <typename T>
  static Document encode(const T &v) {
    Document document;
    static_cast<Value &>(document) = Codec{document.GetAllocator()}.encode(v);
    return document;
  }

  template <typename T>
  outcome::result<T> decode(const Value &j) {
    try {
      return Codec::decode<T>(j);
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }
  }
}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_RPC_JSON_HPP
