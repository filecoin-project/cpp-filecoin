/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_CLIENT_EVENTS_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_CLIENT_EVENTS_HPP

namespace fc::markets::storage::client {

  enum class ClientEvent : uint64_t {
    /* ClientEventOpen indicates a new deal was started */
    ClientEventOpen = 1,

    /**
     * ClientEventFundsInitiated happens when a client has sent a message adding
     * funds to its balance
     */
    ClientEventFundingInitiated,

    /**
     * ClientEventFundsEnsured happens when a client successfully ensures it has
     * funds for a deal
     */
    ClientEventFundsEnsured,

    /**
     * ClientEventDealProposed happens when a new proposal is sent to a provider
     */
    ClientEventDealProposed,

    /**
     * ClientEventDealRejected happens when the provider does not accept a deal
     */
    ClientEventDealRejected,

    /**
     * ClientEventDealAccepted happens when a client receives a response
     * accepting a deal from a provider
     */
    ClientEventDealAccepted,

    /**
     * ClientEventDealPublished happens when a deal is successfully published
     */
    ClientEventDealPublished,

    /**
     * ClientEventDealActivated happens when a deal is successfully activated
     */
    ClientEventDealActivated,

    /** ClientEventFailed happens when a deal terminates in failure */
    ClientEventFailed
  };

}

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_CLIENT_EVENTS_HPP
