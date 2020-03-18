/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PROOF_VERIFIER_HPP
#define CPP_FILECOIN_CORE_PROOF_VERIFIER_HPP

#include "primitives/sector/sector.hpp"

namespace fc::sectorbuilder {
  class ProofVerifier {
   public:
    virtual ~ProofVerifier() = default;

    virtual outcome::result<bool> verifySeal(
        const primitives::sector::SealVerifyInfo &info) const = 0;

    virtual outcome::result<bool> verifyElectionPost(
        const primitives::sector::PoStVerifyInfo &info) const = 0;

    virtual outcome::result<bool> verifyFallbackPost(
        const primitives::sector::PoStVerifyInfo &info) const = 0;
  };
}  // namespace fc::sectorbuilder

#endif  // CPP_FILECOIN_CORE_PROOF_VERIFIER_HPP
