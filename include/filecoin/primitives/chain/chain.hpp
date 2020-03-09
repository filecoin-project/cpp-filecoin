/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_CHAIN_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_CHAIN_HPP

#include "filecoin/common/outcome.hpp"
#include "filecoin/crypto/randomness/randomness_types.hpp"
#include "filecoin/primitives/chain_epoch/chain_epoch.hpp"
#include "filecoin/primitives/tipset/tipset.hpp"

namespace fc::primitives::chain {
  /**
   * @struct Chain is a sequence of tipsets linked together
   */
  struct Chain {
    tipset::Tipset head_tipset;

    outcome::result<std::reference_wrapper<const tipset::Tipset>>
    getTipsetAtEpoch(ChainEpoch epoch) const;

    outcome::result<crypto::randomness::Randomness> getRandomnessAtEpoch(
        ChainEpoch epoch) const;

    ChainEpoch getLatestCheckpoint() const;
  };

  /** @brief Checkpoint represents a particular block to use as a trust anchor
   * in Consensus and ChainSync
   *
   * Note: a Block uniquely identifies a tipset (the parents)
   * from here, we may consider many tipsets that _include_ Block
   * but we must indeed include t and not consider tipsets that
   * fork from Block.Parents, but do not include Block.
   */
  using Checkpoint = std::reference_wrapper<block::BlockHeader>;

  /**
   * @brief SoftCheckpoint is a checkpoint that Filecoin nodes may use as
   * they gain confidence in the blockchain. It is a unilateral checkpoint, and
   * derived algorithmically from notions of probabilistic consensus and
   * finality.
   */
  using SoftCheckpoint = Checkpoint;

  /**
   * @brief TrustedCheckpoint is a Checkpoint that is trusted by the broader
   * Filecoin Network. These TrustedCheckpoints are arrived at through the
   * higher level economic consensus that surrounds Filecoin.
   * TrustedCheckpoints:
   *  - MUST be at least 200,000 blocks old (>1mo)
   *  - MUST be at least
   *  - MUST be widely known and accepted
   *  - MAY ship with Filecoin software implementations
   *  - MAY be propagated through other side-channel systems
   */
  using TrustedCheckpoint = Checkpoint;
}  // namespace fc::primitives::chain

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_CHAIN_HPP
