/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_DRAND_EXAMPLE_EXAMPLE_CONSTANTS_HPP
#define CPP_FILECOIN_CORE_DRAND_EXAMPLE_EXAMPLE_CONSTANTS_HPP

#include <string_view>

namespace fc::drand::example {
  constexpr auto kRounds = 3;

  constexpr std::string_view kSigFile = "signature";
  constexpr std::string_view kPreviousSigFile = "previous";
  constexpr std::string_view kRandomnessFile = "randomness";
  constexpr std::string_view kGroupCoefFile = "coef";
};  // namespace fc::drand::example

#endif  // CPP_FILECOIN_CORE_DRAND_EXAMPLE_EXAMPLE_CONSTANTS_HPP
