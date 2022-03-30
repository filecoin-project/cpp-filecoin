/**
* Copyright Soramitsu Co., Ltd. All Rights Reserved.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "codec/json/coding.hpp"
#include "primitives/cid/cid.hpp"

namespace fc {
  JSON_ENCODE(CID) {
    using codec::json::Set;
    using codec::json::Value;
    OUTCOME_EXCEPT(str, v.toString());
    Value j{rapidjson::kObjectType};
    Set(j, "/", str, allocator);
    return j;
  }

  JSON_DECODE(CID) {
    using codec::json::AsString;
    using codec::json::Get;
    OUTCOME_EXCEPT(cid, CID::fromString(AsString(Get(j, "/"))));
    v = std::move(cid);
  }
}  // namespace fc