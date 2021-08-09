/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "common/math/math.hpp"
#include "primitives/big_int.hpp"

namespace fc::common::smoothing {
  using common::math::kPrecision128;
  using primitives::BigInt;

  /// Q.128 value of 9.25e-4
  static const BigInt kDefaultAlpha("314760000000000000000000000000000000");

  /// Q.128 value of 2.84e-7
  static const BigInt kDefaultBeta("96640100000000000000000000000000");

  struct FilterEstimate {
    BigInt position;
    BigInt velocity;
  };
  CBOR_TUPLE(FilterEstimate, position, velocity)

  BigInt estimate(const FilterEstimate &filter);
  FilterEstimate nextEstimate(const FilterEstimate &previous_estimate,
                              const BigInt &observation,
                              uint64_t delta);
  BigInt extrapolatedCumSumOfRatio(uint64_t delta,
                                   uint64_t start,
                                   const FilterEstimate &num,
                                   const FilterEstimate &den);
}  // namespace fc::common::smoothing
