/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fsm/error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::fsm, FsmError, e) {
  using E = fc::fsm::FsmError;
  switch (e) {
    case E::ENTITY_NOT_TRACKED:
      return "Specified element was not tracked by FSM.";
    case E::ENTITY_ALREADY_BEING_TRACKED:
      return "FSM is tracking the entity's state already.";
    case E::MACHINE_STOPPED:
      return "FSM has been stopped. No more events get processed.";
  }
}
