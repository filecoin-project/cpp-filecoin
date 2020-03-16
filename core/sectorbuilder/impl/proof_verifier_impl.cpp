/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sectorbuilder/impl/proof_verifier_impl.hpp"

#include "proofs/proofs.hpp"

fc::outcome::result<bool> fc::sectorbuilder::ProofVerifierImpl::verifySeal(
    const fc::primitives::sector::SealVerifyInfo &info) const {
  return fc::proofs::Proofs::verifySeal(info);
}

fc::outcome::result<bool>
fc::sectorbuilder::ProofVerifierImpl::verifyElectionPost(
    const fc::primitives::sector::PoStVerifyInfo &info) const {
  return verifyPost(info);
}

fc::outcome::result<bool>
fc::sectorbuilder::ProofVerifierImpl::verifyFallbackPost(
    const fc::primitives::sector::PoStVerifyInfo &info) const {
  return verifyPost(info);
}

fc::outcome::result<bool> fc::sectorbuilder::ProofVerifierImpl::verifyPost(
    fc::primitives::sector::PoStVerifyInfo info) const {
  info.randomness[31] = 0;

  return fc::proofs::Proofs::verifyPoSt(info);
}
