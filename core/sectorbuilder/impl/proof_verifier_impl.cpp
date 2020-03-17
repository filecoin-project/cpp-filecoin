/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sectorbuilder/impl/proof_verifier_impl.hpp"

#include "proofs/proofs.hpp"

namespace fc::sectorbuilder {

  outcome::result<bool> ProofVerifierImpl::verifySeal(
      const primitives::sector::SealVerifyInfo &info) const {
    return proofs::Proofs::verifySeal(info);
  }

  outcome::result<bool> ProofVerifierImpl::verifyElectionPost(
      const primitives::sector::PoStVerifyInfo &info) const {
    return verifyPost(info);
  }

  outcome::result<bool> ProofVerifierImpl::verifyFallbackPost(
      const primitives::sector::PoStVerifyInfo &info) const {
    return verifyPost(info);
  }

  outcome::result<bool> ProofVerifierImpl::verifyPost(
      primitives::sector::PoStVerifyInfo info) const {
    info.randomness[31] = 0;

    return proofs::Proofs::verifyPoSt(info);
  }
}  // namespace fc::sectorbuilder
