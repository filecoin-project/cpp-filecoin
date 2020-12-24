/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/smoothing/alpha_beta_filter.hpp"

namespace fc::common::smoothing {

  FilterEstimate nextEstimate(const FilterEstimate &previous_estimate,
                              const BigInt &observation,
                              uint64_t delta) {
    // to Q.128 format
    const BigInt delta_t = BigInt(delta) << kPrecision;
    // Q.128 * Q.128 => Q.256
    BigInt delta_x = delta_t * previous_estimate.velocity;
    // Q.256 => Q.128
    delta_x >>= kPrecision;
    BigInt position = previous_estimate.position + delta_x;

    // observation Q.0 => Q.128
    const BigInt residual = (observation << kPrecision) - position;
    // Q.128 * Q.128 => Q.256
    BigInt revision_x = kDefaultAlpha * residual;
    // Q.256 => Q.128
    revision_x >>= kPrecision;
    position += revision_x;

    // Q.128 * Q.128 => Q.256
    BigInt revision_v = kDefaultBeta * residual;
    // Q.256 / Q.128 => Q.128
    revision_v = bigdiv(revision_v, delta_t);
    const BigInt velocity = previous_estimate.velocity + revision_v;

    return {position, velocity};
  }

}  // namespace fc::common::smoothing
