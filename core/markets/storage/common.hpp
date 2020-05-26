/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_COMMON_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_COMMON_HPP

#define FSM_SEND(deal, event) OUTCOME_EXCEPT(fsm_->send(deal, event))

#define SELF_FSM_SEND(deal, event) OUTCOME_EXCEPT(self->fsm_->send(deal, event))

#endif  // CPP_FILECOIN_MARKETS_STORAGE_COMMON_HPP
