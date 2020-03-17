/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PROOF_VERIFIER_IMPL_HPP
#define CPP_FILECOIN_CORE_PROOF_VERIFIER_IMPL_HPP

#include "sectorbuilder/proof_verifier.hpp"

namespace fc::sectorbuilder {
  class ProofVerifierImpl : public ProofVerifier {
   public:
    outcome::result<bool> verifySeal(
        const primitives::sector::SealVerifyInfo &info) const override;

    outcome::result<bool> verifyElectionPost(
        const primitives::sector::PoStVerifyInfo &info) const override;

    outcome::result<bool> verifyFallbackPost(
        const primitives::sector::PoStVerifyInfo &info) const override;

   private:
    outcome::result<bool> verifyPost(
        primitives::sector::PoStVerifyInfo info) const;
  };

}  // namespace fc::sectorbuilder

#endif  // CPP_FILECOIN_CORE_PROOF_VERIFIER_IMPL_HPP
