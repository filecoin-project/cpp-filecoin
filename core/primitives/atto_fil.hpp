/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/validate/with.hpp"
#include "const.hpp"

namespace fc::primitives {
  // TODO: fractions
  // TODO: suffix ("fil", "attofil", "afil")
  struct AttoFil {
    BigInt fil;
    AttoFil() = default;
    TokenAmount atto() const {
      return fil * kFilecoinPrecision;
    }
  };
  CLI_VALIDATE(AttoFil) {
    validateWith(out, values, [](const std::string &value) {
      AttoFil atto;
      atto.fil = BigInt{value};
      return atto;
    });
  }
}  // namespace fc::primitives
