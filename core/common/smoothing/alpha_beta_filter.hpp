/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/big_int.hpp"

namespace fc::common::smoothing {
  using primitives::BigInt;

  static constexpr uint kPrecision = 128;

  /// Q.128 value of 9.25e-4
  static const BigInt kDefaultAlpha("314760000000000000000000000000000000");

  /// Q.128 value of 2.84e-7
  static const BigInt kDefaultBeta("96640100000000000000000000000000");

  struct FilterEstimate {
    BigInt position;
    BigInt velocity;
  };
  CBOR_TUPLE(FilterEstimate, position, velocity)

  FilterEstimate nextEstimate(const FilterEstimate &previous_estimate,
                              const BigInt &observation,
                              uint64_t delta);
}  // namespace fc::common::smoothing
