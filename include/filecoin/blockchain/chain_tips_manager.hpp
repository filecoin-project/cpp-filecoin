/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_CHAIN_TIPS_MANAGER_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_CHAIN_TIPS_MANAGER_HPP

#include "filecoin/primitives/tipset/tipset.hpp"

namespace fc::primitives::chain {

  class ChainTipsManager {
    virtual ~ChainTipsManager() = default;
    // Returns the ticket that is at round 'r' in the chain behind 'head'
    virtual outcome::result<std::reference_wrapper<const ticket::Ticket>>
    getTicketFromRound(const tipset::Tipset &tipset, uint64_t rount) const = 0;

    // Returns the tipset that contains round r (Note: multiple rounds' worth of
    // tickets may exist within a single block due to losing tickets being added
    // to the eventually successfully generated block)
    virtual outcome::result<std::reference_wrapper<const tipset::Tipset>>
    getTipsetFromRound(const tipset::Tipset &tipset, uint64_t round) const = 0;

    // GetBestTipset returns the best known tipset. If the 'best' tipset hasn't
    // changed, then this will return the previous best tipset.
    virtual outcome::result<std::reference_wrapper<const tipset::Tipset>>
    getBestTipset() const = 0;

    // Adds the losing ticket to the chaintips manager so that blocks can be
    // mined on top of it
    virtual outcome::result<void> addLosingTicket(
        const tipset::Tipset &parent, const ticket::Ticket &ticket) = 0;

  };
}  // namespace fc::primitives::chain

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_CHAIN_TIPS_MANAGER_HPP
