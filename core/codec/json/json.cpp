/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/json/json.hpp"

<<<<<<< HEAD
=======
#include <rapidjson/reader.h>
>>>>>>> master
#include <rapidjson/writer.h>
#include <cppcodec/base64_rfc4648.hpp>

#include "common/span.hpp"

namespace fc::codec::json {
<<<<<<< HEAD
=======
  using rapidjson::ParseFlag;
>>>>>>> master
  using rapidjson::StringBuffer;
  using base64 = cppcodec::base64_rfc4648;

  Outcome<Document> parse(std::string_view input) {
    Document doc;
<<<<<<< HEAD
    doc.Parse(input.data(), input.size());
=======
    doc.Parse<ParseFlag::kParseNumbersAsStringsFlag>(input.data(),
                                                     input.size());
>>>>>>> master
    if (doc.HasParseError()) {
      return {};
    }
    return std::move(doc);
  }

  Outcome<Document> parse(BytesIn input) {
    return parse(common::span::bytestr(input));
  }

  Outcome<Buffer> format(JIn j) {
    StringBuffer buffer;
    rapidjson::Writer<StringBuffer> writer{buffer};
    if (j->Accept(writer)) {
      std::string_view s{buffer.GetString(), buffer.GetSize()};
      return Buffer{common::span::cbytes(s)};
    }
    return {};
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
<<<<<<< HEAD
    if (j->IsInt64()) {
      return j->GetInt64();
=======
    if (j->IsString()) {
      return strtoll(j->GetString(), nullptr, 10);
>>>>>>> master
    }
    return {};
  }

  Outcome<uint64_t> jUint(JIn j) {
<<<<<<< HEAD
    if (j->IsUint64()) {
      return j->GetUint64();
=======
    if (j->IsString()) {
      return strtoull(j->GetString(), nullptr, 10);
    }
    return {};
  }

  Outcome<BigInt> jBigInt(JIn j) {
    if (j->IsString()) {
      return BigInt{j->GetString()};
>>>>>>> master
    }
    return {};
  }
}  // namespace fc::codec::json
