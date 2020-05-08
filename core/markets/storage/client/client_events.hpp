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

    /* Emitted on stream open error */
    ClientEventOpenStreamError,

    /**
     * ClientEventEnsureFundsFailed happens when attempting to ensure the client
     * has enough funds available fails
     */
    ClientEventEnsureFundsFailed,

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
     * ClientEventWriteProposalFailed indicates an attempt to send a deal
     * proposal to a provider failed
     */
    ClientEventWriteProposalFailed,

    /**
     * ClientEventDealProposed happens when a new proposal is sent to a provider
     */
    ClientEventDealProposed,

    /**
     * ClientEventDealStreamLookupErrored the deal stream for a deal could not
     * be found
     */
    ClientEventDealStreamLookupErrored,

    /**
     * ClientEventReadResponseFailed means a network error occurred reading a
     * deal response
     */
    ClientEventReadResponseFailed,

    /**
     * ClientEventResponseVerificationFailed means a response was not verified
     */
    ClientEventResponseVerificationFailed,

    /**
     * ClientEventResponseDealDidNotMatch means a response was sent for the
     * wrong deal
     */
    ClientEventResponseDealDidNotMatch,

    /**
     * ClientEventStreamCloseError happens when an attempt to close a deals
     * stream fails
     */
    ClientEventStreamCloseError,

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
     * ClientEventDealPublishFailed happens when a client cannot verify a deal
     * was published
     */
    ClientEventDealPublishFailed,

    /**
     * ClientEventDealPublished happens when a deal is successfully published
     */
    ClientEventDealPublished,

    /**
     * ClientEventDealActivationFailed happens when a client cannot verify a
     * deal was activated
     */
    ClientEventDealActivationFailed,

    /**
     * ClientEventDealActivated happens when a deal is successfully activated
     */
    ClientEventDealActivated,

    /** ClientEventFailed happens when a deal terminates in failure */
    ClientEventFailed
  };

}

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_CLIENT_EVENTS_HPP
