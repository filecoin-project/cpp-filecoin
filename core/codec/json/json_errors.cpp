/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/json/json_errors.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::codec::json, JsonError, e) {
  using E = fc::codec::json::JsonError;
  switch (e) {
    case E::kWrongLength:
      return "wrong length";
    case E::kWrongEnum:
      return "wrong enum";
    case E::kWrongType:
      return "wrong type";
    case E::kOutOfRange:
      return "out of range";
    case E::kWrongParams:
      return "wrong params";
  }

  return "unknown JsonError error code";
}
