/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::codec::json  {
  enum class JsonError {
    kWrongLength = 1,
    kWrongEnum,
    kWrongType,
    kOutOfRange,
    kWrongParams,
  };
}  // namespace fc::api

OUTCOME_HPP_DECLARE_ERROR(fc::codec::json , JsonError);
