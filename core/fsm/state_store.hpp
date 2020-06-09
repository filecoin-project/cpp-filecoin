/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_FSM_STATE_STORE_HPP
#define CPP_FILECOIN_CORE_FSM_STATE_STORE_HPP

#include "primitives/cid/cid.hpp"

namespace fc::fsm {
  /**
   * Interface to store FSM states
   */
  template <typename Key, typename State>
  class StateStore {
   public:
    virtual ~StateStore() = default;

    /**
     * Get FSM state by cid
     * @param cid
     * @return
     */
    virtual outcome::result<State> get(const Key &key) const = 0;
  };
}  // namespace fc::fsm

#endif  // CPP_FILECOIN_CORE_FSM_STATE_STORE_HPP
