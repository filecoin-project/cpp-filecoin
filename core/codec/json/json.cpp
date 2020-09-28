/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/json/json.hpp"

#include <rapidjson/writer.h>
#include <cppcodec/base64_rfc4648.hpp>

#include "common/span.hpp"

namespace fc::codec::json {
  using base64 = cppcodec::base64_rfc4648;

  Outcome<Document> parse(std::string_view input) {
    Document doc;
    doc.Parse(input.data(), input.size());
    if (doc.HasParseError()) {
      return {};
    }
    return std::move(doc);
  }

  Outcome<Document> parse(BytesIn input) {
    return parse(common::span::bytestr(input));
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

  Outcome<Buffer> jUnhex(JIn j) {
    OUTCOME_TRY(str, jStr(j));
    return Buffer::fromHex(str);
  }

  Outcome<Buffer> jBytes(JIn j) {
    OUTCOME_TRY(str, jStr(j));
    return Buffer{base64::decode(str)};
  }

  Outcome<CID> jCid(JIn j) {
    OUTCOME_TRY(j2, jGet(j, "/"));
    OUTCOME_TRY(str, jStr(j2));
    return CID::fromString(std::string{str});
  }

  Outcome<int64_t> jInt(JIn j) {
    if (j->IsInt64()) {
      return j->GetInt64();
    }
    return {};
  }

  Outcome<uint64_t> jUint(JIn j) {
    if (j->IsUint64()) {
      return j->GetUint64();
    }
    return {};
  }
}  // namespace fc::codec::json
