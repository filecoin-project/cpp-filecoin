/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/vrf_types.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::crypto::vrf, VRFError, e) {
  using fc::crypto::vrf::VRFError;
  switch (e) {
    case (VRFError::kMinerAddressNotId):
      return "miner address has to be of ID type to calculate hash";
    case VRFError::kVerificationFailed:
      return "VRF verification failed";
    case VRFError::kSignFailed:
      return "VRF message sign failed";
    case VRFError::kAddressIsNotBLS:
      return "cannot make VRF hash on address, which is not BLS";
  }

  return "unknown error";
}
