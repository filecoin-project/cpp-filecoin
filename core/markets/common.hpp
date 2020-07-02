/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_COMMON_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_COMMON_HPP

#define FSM_SEND(deal, event) OUTCOME_EXCEPT(fsm_->send(deal, event))

#define SELF_FSM_SEND(deal, event) OUTCOME_EXCEPT(self->fsm_->send(deal, event))

namespace fc::markets {
  using common::Logger;
  using common::libp2p::CborStream;

  /**
   * Closes stream and handles close result
   * @param stream to close
   */
  inline void closeStreamGracefully(const std::shared_ptr<CborStream> &stream,
                                    const Logger &logger) {
    if (!stream->stream()->isClosed()) {
      stream->stream()->close([logger](outcome::result<void> res) {
        if (res.has_error()) {
          logger->error("Close stream error " + res.error().message());
        }
      });
    }
  }

}  // namespace fc::markets

#endif  // CPP_FILECOIN_MARKETS_STORAGE_COMMON_HPP
