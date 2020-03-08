/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/crypto/vrf/vrf_types.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::crypto::vrf, VRFError, e) {
  using fc::crypto::vrf::VRFError;
  switch (e) {
    case (VRFError::MINER_ADDRESS_NOT_ID):
      return "miner address has to be of ID type to calculate hash";
    case VRFError::VERIFICATION_FAILED:
      return "VRF verification failed";
    case VRFError::SIGN_FAILED:
      return "VRF message sign failed";
    case VRFError::ADDRESS_IS_NOT_BLS:
      return "cannot make VRF hash on address, which is not BLS";
  }

  return "unknown error";
}
