/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/json_errors.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::api, JsonError, e) {
  using E = fc::api::JsonError;
  switch (e) {
    case E::WRONG_LENGTH:
      return "wrong length";
    case E::WRONG_ENUM:
      return "wrong enum";
    case E::WRONG_TYPE:
      return "wrong type";
    case E::OUT_OF_RANGE:
      return "out of range";
  }
}
