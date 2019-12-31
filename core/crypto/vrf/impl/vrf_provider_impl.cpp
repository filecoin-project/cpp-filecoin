/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/impl/vrf_provider_impl.hpp"

#include "common/outcome.hpp"

namespace fc::crypto::vrf {

  VRFProviderImpl::VRFProviderImpl(
      std::shared_ptr<bls::BlsProvider> bls_provider)
      : bls_provider_(std::move(bls_provider)) {}

  outcome::result<VRFResult> VRFProviderImpl::generateVRF(
      const VRFSecretKey &worker_secret_key, const VRFHash &msg) const {
    auto &&result = bls_provider_->sign(msg, worker_secret_key);
    if (!result) {
      return VRFError::SIGN_FAILED;
    }
    return result.value();
  }

  outcome::result<bool> VRFProviderImpl::verifyVRF(
      const VRFPublicKey &worker_public_key,
      const VRFHash &msg,
      const VRFProof &vrf_proof) const {
    auto &&res =
        bls_provider_->verifySignature(msg, vrf_proof, worker_public_key);
    if (res.has_failure()) {
      return VRFError::VERIFICATION_FAILED;
    }
    return res.value();
  }
}  // namespace fc::crypto::vrf

OUTCOME_CPP_DEFINE_CATEGORY(fc::crypto::vrf, VRFError, e) {
  using fc::crypto::vrf::VRFError;
  switch (e) {
    case (VRFError::MINER_ADDRESS_NOT_ID):
      return "miner address has to be of ID type to calculate hash";
    case VRFError::VERIFICATION_FAILED:
      return "VRF verification failed";
    case VRFError::SIGN_FAILED:
      return "VRF message sign failed";
  }

  return "unknown error";
}
