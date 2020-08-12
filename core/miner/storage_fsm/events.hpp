/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_EVENTS_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_EVENTS_HPP

namespace fc::mining {

  class Events {
   public:
    using HeightHandler =
        std::function<outcome::result<void>(const TipsetToken &, ChainEpoch)>;
    using RevertHandler =
        std::function<outcome::result<void>(const TipsetToken &)>;

    virtual ~Events() = default;

    /**
     * @param confidence = current_height - tipset.height
     */
    virtual outcome::result<void> chainAt(HeightHandler,
                                          RevertHandler,
                                          int confidence,
                                          ChainEpoch height) = 0;
  };

}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_EVENTS_HPP
