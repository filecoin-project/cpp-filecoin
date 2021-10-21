/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/json/json.hpp"

#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <cppcodec/base64_rfc4648.hpp>

#include "common/span.hpp"

namespace fc::codec::json {
  using rapidjson::ParseFlag;
  using rapidjson::StringBuffer;
  using base64 = cppcodec::base64_rfc4648;

  Outcome<Document> parse(std::string_view input) {
    Document doc;
    doc.Parse<ParseFlag::kParseNumbersAsStringsFlag>(input.data(),
                                                     input.size());
    if (doc.HasParseError()) {
      return {};
    }
    return std::move(doc);
  }

  Outcome<Document> parse(BytesIn input) {
    return parse(common::span::bytestr(input));
  }

  Outcome<Bytes> format(JIn j) {
    StringBuffer buffer;
    rapidjson::Writer<StringBuffer> writer{buffer};
    if (j->Accept(writer)) {
      std::string_view s{buffer.GetString(), buffer.GetSize()};
      return Bytes(s.begin(), s.end());
    }
    return {};
  }

  Outcome<Bytes> format(Document &&doc) {
    return format(&doc);
  }

  Outcome<JIn> jGet(JIn j, std::string_view key) {
    if (j->IsObject()) {
      auto it{j->FindMember(key.data())};
      if (it != j->MemberEnd()) {
        return &it->value;
      }
    }
    return {};
  }

  Outcome<std::string_view> jStr(JIn j) {
    if (j->IsString()) {
      return std::string_view{j->GetString(), j->GetStringLength()};
    }
    return {};
  }

  Outcome<Bytes> jUnhex(JIn j) {
    OUTCOME_TRY(str, jStr(j));
    return common::unhex(str);
  }

  Outcome<Bytes> jBytes(JIn j) {
    OUTCOME_TRY(str, jStr(j));
    return base64::decode(str);
  }

  Outcome<CID> jCid(JIn j) {
    OUTCOME_TRY(j2, jGet(j, "/"));
    OUTCOME_TRY(str, jStr(j2));
    return CID::fromString(std::string{str});
  }

  Outcome<int64_t> jInt(JIn j) {
    if (j->IsString()) {
      return strtoll(j->GetString(), nullptr, 10);
    }
    return {};
  }

  Outcome<uint64_t> jUint(JIn j) {
    if (j->IsString()) {
      return strtoull(j->GetString(), nullptr, 10);
    }
    return {};
  }

  Outcome<BigInt> jBigInt(JIn j) {
    if (j->IsString()) {
      return BigInt{j->GetString()};
    }
    return {};
  }
}  // namespace fc::codec::json
