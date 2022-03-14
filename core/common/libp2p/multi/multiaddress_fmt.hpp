/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/fmt/fmt.h>
#include <libp2p/multi/multiaddress.hpp>

template <>
struct fmt::formatter<libp2p::multi::Multiaddress>
    : formatter<std::string_view> {
  template <typename C>
  auto format(const libp2p::multi::Multiaddress &address, C &ctx) {
    return formatter<std::string_view>::format(address.getStringAddress(), ctx);
  }
};
