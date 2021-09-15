/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <rapidjson/document.h>

#include "common/buffer.hpp"
#include "common/outcome2.hpp"
#include "primitives/big_int.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::codec::json {
  using fc::primitives::BigInt;
  using rapidjson::Document;
  using rapidjson::Value;
  using JIn = const Value *;

  Outcome<Document> parse(std::string_view input);

  Outcome<Document> parse(BytesIn input);

  Outcome<Buffer> format(JIn j);
  Outcome<Buffer> format(Document &&doc);

  Outcome<JIn> jGet(JIn j, std::string_view key);

  Outcome<std::string_view> jStr(JIn j);

  Outcome<Buffer> jUnhex(JIn j);

  Outcome<Buffer> jBytes(JIn j);

  Outcome<CID> jCid(JIn j);

  template <typename F>
  inline auto jList(JIn j, const F &f) {
    using T = std::invoke_result_t<F, JIn>;
    Outcome<std::vector<T>> list;
    if (j && j->IsArray()) {
      list.emplace();
      list->reserve(j->Size());
      for (const auto &it : j->GetArray()) {
        list->push_back(f(&it));
      }
    }
    return list;
  }

  Outcome<int64_t> jInt(JIn j);

  Outcome<uint64_t> jUint(JIn j);

  Outcome<BigInt> jBigInt(JIn j);
}  // namespace fc::codec::json
