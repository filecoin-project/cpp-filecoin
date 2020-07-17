/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_REQUEST_VALIDATOR_HPP
#define CPP_FILECOIN_REQUEST_VALIDATOR_HPP

#include <libp2p/peer/peer_info.hpp>
#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/ipld/selector.hpp"

namespace fc::data_transfer {

  using libp2p::peer::PeerInfo;
  using storage::ipld::Selector;

  /**
   * RequestValidator is an interface implemented by the client of the data
   * transfer module to validate requests
   */
  class RequestValidator {
   public:
    virtual ~RequestValidator() = default;

    /**
     * ValidatePush validates a push request received from the peer that will
     * send data
     */
    virtual outcome::result<void> validatePush(
        const PeerInfo &sender,
        std::vector<uint8_t> voucher,
        CID base_cid,
        std::shared_ptr<Selector> selector) = 0;
    /**
     * ValidatePull validates a pull request received from the peer thatF will
     * receive data
     */
    virtual outcome::result<void> validatePull(
        const PeerInfo &receiver,
        std::vector<uint8_t> voucher,
        CID base_cid,
        std::shared_ptr<Selector> selector) = 0;
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_REQUEST_VALIDATOR_HPP
