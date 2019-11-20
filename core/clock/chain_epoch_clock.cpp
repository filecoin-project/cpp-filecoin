/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/chain_epoch_clock.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::clock, EpochAtTimeError, e) {
  using fc::clock::EpochAtTimeError;
  switch (e) {
    case EpochAtTimeError::BEFORE_GENESIS:
      return "Input time is before genesis time";
    default:
      return "Unknown error";
  }
}
