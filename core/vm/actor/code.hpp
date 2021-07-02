/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

namespace fc {
  struct ActorCodeCid : std::string_view {};
  constexpr bool operator==(const ActorCodeCid &l, const ActorCodeCid &r) {
    return (const std::string_view &)l == (const std::string_view &)r;
  }
}  // namespace fc
