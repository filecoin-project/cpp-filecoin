/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/json_errors.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::api, JsonError, e) {
  using E = fc::api::JsonError;
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
