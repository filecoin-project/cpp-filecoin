/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CHAIN_STATE_MANAGER_STATE_MANAGER_HPP
#define CPP_FILECOIN_CORE_CHAIN_STATE_MANAGER_STATE_MANAGER_HPP

#include <libp2p/multi/content_identifier.hpp>

#include "common/outcome.hpp"
#include "crypto/randomness/randomness_provider.hpp"
#include "primitives/chain/message.hpp"
#include "primitives/chain/message_receipt.hpp"

namespace fc::chain::stmgr {
  class StateManager {
   public:
    using CID = libp2p::multi::ContentIdentifier;

    virtual ~StateManager() = default;

    // rand is random provider and not required to be passed here
    virtual outcome::result<primitives::chain::MessageReceipt> callRaw(
        const primitives::chain::Message &message,
        const CID &base_state,
        uint64_t base_height) = 0;
  };
}  // namespace fc::chain::stmgr

#endif  // CPP_FILECOIN_CORE_CHAIN_STATE_MANAGER_STATE_MANAGER_HPP
