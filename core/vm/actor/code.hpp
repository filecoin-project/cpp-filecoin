/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

#include "common/cmp.hpp"

namespace fc {
  struct ActorCodeCid : std::string_view {};
  constexpr bool operator==(const ActorCodeCid &l, const ActorCodeCid &r) {
    return static_cast<const std::string_view &>(l)
           == static_cast<const std::string_view &>(r);
  }
  FC_OPERATOR_NOT_EQUAL(ActorCodeCid)
}  // namespace fc
