/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc::markets::storage::provider {

  /**
   * ProviderEvent is an event that occurs in a deal lifecycle on the provider
   */
  enum class ProviderEvent : uint64_t {
    /** ProviderEventOpen indicates a new deal proposal has been received */
    ProviderEventOpen = 1,

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
     * ProviderEventDataTransferInitiated happens when a data transfer starts
     */
    ProviderEventDataTransferInitiated,

    /**
     * ProviderEventDataTransferCompleted happens when a data transfer is
     * successful
     */
    ProviderEventDataTransferCompleted,

    /**
     * ProviderEventVerifiedData happens when received data is verified as
     * matching the pieceCID in a deal proposal
     */
    ProviderEventVerifiedData,

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
     * ProviderEventDealHandedOff happens when a deal is successfully handed off
     * to the node for processing in a sector
     */
    ProviderEventDealHandedOff,

    /**
     * ProviderEventDealActivated happens when a deal is successfully activated
     * and commited to a sector
     */
    ProviderEventDealActivated,

    /** ProviderEventDealCompleted happens when a deal completes successfully */
    ProviderEventDealCompleted,

    /**
     * ProviderEventFailed indicates a deal has failed and should no longer be
     * processed
     */
    ProviderEventFailed
  };

}  // namespace fc::markets::storage::provider
