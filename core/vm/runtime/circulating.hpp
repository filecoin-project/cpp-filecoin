/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "fwd.hpp"
#include "primitives/types.hpp"

namespace fc::vm {
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using StateTreePtr = const std::shared_ptr<state::StateTree> &;

  struct Circulating {
    static outcome::result<std::shared_ptr<Circulating>> make(
        IpldPtr ipld, const CID &genesis);
    outcome::result<TokenAmount> vested(ChainEpoch epoch) const;
    outcome::result<TokenAmount> circulating(StateTreePtr state_tree,
                                             ChainEpoch epoch) const;

    TokenAmount genesis;
  };
}  // namespace fc::vm
