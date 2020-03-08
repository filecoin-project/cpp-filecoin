/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/crypto/vrf/impl/vrf_provider_impl.hpp"
#include "filecoin/crypto/vrf/vrf_hash_encoder.hpp"

#include "filecoin/common/outcome.hpp"

namespace fc::crypto::vrf {

  VRFProviderImpl::VRFProviderImpl(
      std::shared_ptr<bls::BlsProvider> bls_provider)
      : bls_provider_(std::move(bls_provider)) {
    BOOST_ASSERT_MSG(bls_provider_ != nullptr, "bls provider is nullptr");
  }

  outcome::result<VRFResult> VRFProviderImpl::computeVRF(
      const VRFSecretKey &secret_key, const VRFParams &params) const {
    OUTCOME_TRY(hash, encodeVrfParams(params));

    auto &&result = bls_provider_->sign(hash, secret_key);
    if (!result) {
      return VRFError::SIGN_FAILED;
    }
    return result.value();
  }

  outcome::result<bool> VRFProviderImpl::verifyVRF(
      const VRFPublicKey &public_key,
      const VRFParams &params,
      const VRFProof &proof) const {
    OUTCOME_TRY(hash, encodeVrfParams(params));
    auto &&res = bls_provider_->verifySignature(hash, proof, public_key);
    if (res.has_failure()) {
      return VRFError::VERIFICATION_FAILED;
    }
    return res.value();
  }
}  // namespace fc::crypto::vrf
