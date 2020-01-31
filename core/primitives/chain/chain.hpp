/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_CHAIN_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_CHAIN_HPP

#include "crypto/randomness/randomness_types.hpp"
#include "primitives/tipset/tipset.hpp"
#include "common/outcome.hpp"

namespace fc::primitives::chain {
  struct Chain {
    tipset::Tipset head_tipset;

    outcome::result<std::reference_wrapper<const tipset::Tipset>>
    getTipsetAtEpoch(crypto::randomness::ChainEpoch epoch) const;

    outcome::result<crypto::randomness::Randomness>
    getRandomnessAtEpoch(crypto::randomness::ChainEpoch epoch) const;

    crypto::randomness::ChainEpoch getLatestCheckpoint() const;
  };

}  // namespace fc::primitives::chain

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_CHAIN_HPP
