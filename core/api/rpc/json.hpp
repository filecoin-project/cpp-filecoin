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

#define ENCODE(type) Value encode(const type &value)

#define DECODE(type) static void decode(type &value, const Value &encoded)

#define SET(key, value) encoded.AddMember(encode(key), value, allocator)

namespace fc::api {
  using primitives::ticket::Ticket;
  using rapidjson::Document;
  using rapidjson::Value;
  using base64 = cppcodec::base64_rfc4648;

  struct Codec {
    rapidjson::MemoryPoolAllocator<> &allocator;

    static std::string AsString(const Value &encoded) {
      if (!encoded.IsString()) {
        outcome::raise(JsonError::WRONG_TYPE);
      }
      return {encoded.GetString(), encoded.GetStringLength()};
    }

    static const Value &Get(const Value &encoded, const char *key) {
      if (!encoded.IsObject()) {
        outcome::raise(JsonError::WRONG_TYPE);
      }
      auto it = encoded.FindMember(key);
      if (it == encoded.MemberEnd()) {
        outcome::raise(JsonError::OUT_OF_RANGE);
      }
      return it->value;
    }

    static auto decodeBase64(const Value &encoded) {
      return base64::decode(AsString(encoded));
    }


    ENCODE(std::string) {
      return {value.data(),
              static_cast<rapidjson::SizeType>(value.size()),
              allocator};
    }

    ENCODE(gsl::span<const uint8_t>) {
      return encode(base64::encode(value.data(), value.size()));
    }

    template <size_t N>
    DECODE(std::array<uint8_t COMMA N>) {
      auto bytes = decodeBase64(encoded);
      if (bytes.size() != N) {
        outcome::raise(JsonError::WRONG_LENGTH);
      }
      std::copy(bytes.begin(), bytes.end(), value.begin());
    }

    ENCODE(CID) {
      OUTCOME_EXCEPT(str, value.toString());
      Value encoded{rapidjson::kObjectType};
      SET("/", encode(str));
      return encoded;
    }

    DECODE(CID) {
      OUTCOME_EXCEPT(cid, CID::fromString(AsString(Get(encoded, "/"))));
      value = std::move(cid);
    }

    ENCODE(Ticket) {
      Value encoded{rapidjson::kObjectType};
      SET("VRFProof", encode(gsl::make_span(value.bytes)));
      return encoded;
    }

    DECODE(Ticket) {
      decode(value.bytes, Get(encoded, "VRFProof"));
    }
  };

  template <typename T>
  static Document encode(const T &value) {
    Document document;
    static_cast<Value &>(document) =
        Codec{document.GetAllocator()}.encode(value);
    return document;
  }

  template <typename T>
  outcome::result<T> decode(const Value &encoded) {
    try {
      T value;
      Codec::decode(value, encoded);
      return std::move(value);
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }
  }
}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_RPC_JSON_HPP
