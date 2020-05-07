/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_PROVIDER_PROVIDER_EVENTS_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_PROVIDER_PROVIDER_EVENTS_HPP

namespace fc::markets::storage::provider {

  /**
   * ProviderEvent is an event that occurs in a deal lifecycle on the provider
   */
  enum class ProviderEvent : uint64_t {
    /** ProviderEventOpen indicates a new deal proposal has been received */
    ProviderEventOpen = 1,

    /**
     * ProviderEventNodeErrored indicates an error happened talking to the node
     * implementation
     */
    ProviderEventNodeErrored,

    /**
     * ProviderEventDealRejected happens when a deal proposal is rejected for
     * not meeting criteria
     */
    ProviderEventDealRejected,

    /**
     * ProviderEventDealAccepted happens when a deal is accepted based on
     * provider criteria
     */
    ProviderEventDealAccepted,

    /**
     * ProviderEventWaitingForManualData happens when an offline deal proposal
     * is accepted, meaning the provider must wait until it receives data
     * manually
     */
    ProviderEventWaitingForManualData,

    /**
     * ProviderEventInsufficientFunds indicates not enough funds available for a
     * deal
     */
    ProviderEventInsufficientFunds,

    /**
     * ProviderEventFundingInitiated indicates provider collateral funding has
     * been initiated
     */
    ProviderEventFundingInitiated,

    /**
     * ProviderEventFunded indicates provider collateral has appeared in the
     * storage market balance
     */
    ProviderEventFunded,

    /**
     * ProviderEventDataTransferFailed happens when an error occurs transferring
     * data
     */
    ProviderEventDataTransferFailed,

    /**
     * ProviderEventDataTransferInitiated happens when a data transfer starts
     */
    ProviderEventDataTransferInitiated,

    /**
     * ProviderEventDataTransferCompleted happens when a data transfer is
     * successful
     */
    ProviderEventDataTransferCompleted,

    /**
     * ProviderEventManualDataReceived happens when data is received manually
     * for an offline deal
     */
    ProviderEventManualDataReceived,

    /**
     * ProviderEventGeneratePieceCIDFailed happens when generating a piece cid
     * from received data errors
     */
    ProviderEventGeneratePieceCIDFailed,

    /**
     * ProviderEventVerifiedData happens when received data is verified as
     * matching the pieceCID in a deal proposal
     */
    ProviderEventVerifiedData,

    /**
     * ProviderEventSendResponseFailed happens when a response cannot be sent to
     * a deal
     */
    ProviderEventSendResponseFailed,

    /**
     * ProviderEventDealPublishInitiated happens when a provider has sent a
     * PublishStorageDeals message to the chain
     */
    ProviderEventDealPublishInitiated,

    /**
     * ProviderEventDealPublished happens when a deal is successfully published
     */
    ProviderEventDealPublished,

    /**
     * ProviderEventDealPublishError happens when PublishStorageDeals returns a
     * non-ok exit code
     */
    ProviderEventDealPublishError,

    /**
     * ProviderEventFileStoreErrored happens when an error occurs accessing the
     * filestore
     */
    ProviderEventFileStoreErrored,

    /**
     * ProviderEventDealHandoffFailed happens when an error occurs handing off a
     * deal with OnDealComplete
     */
    ProviderEventDealHandoffFailed,

    /**
     * ProviderEventDealHandedOff happens when a deal is successfully handed off
     * to the node for processing in a sector
     */
    ProviderEventDealHandedOff,

    /**
     * ProviderEventDealActivationFailed happens when an error occurs activating
     * a deal
     */
    ProviderEventDealActivationFailed,

    /**
     * ProviderEventUnableToLocatePiece happens when an attempt to learn the
     * location of a piece from the node fails
     */
    ProviderEventUnableToLocatePiece,

    /**
     * ProviderEventDealActivated happens when a deal is successfully activated
     * and commited to a sector
     */
    ProviderEventDealActivated,

    /**
     * ProviderEventPieceStoreErrored happens when an attempt to save data in
     * the piece store errors
     */
    ProviderEventPieceStoreErrored,

    /**
     * ProviderEventReadMetadataErrored happens when an error occurs reading
     * recorded piece metadata
     */
    ProviderEventReadMetadataErrored,

    /** ProviderEventDealCompleted happens when a deal completes successfully */
    ProviderEventDealCompleted,

    /**
     * ProviderEventFailed indicates a deal has failed and should no longer be
     * processed
     */
    ProviderEventFailed
  };

}  // namespace fc::markets::storage::provider

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_PROVIDER_PROVIDER_EVENTS_HPP
