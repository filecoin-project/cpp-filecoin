/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// TODO: config
namespace fc {
  constexpr auto kBlockGasLimit{10000000000};
  constexpr auto kBlockMessageLimit{10000};
  constexpr auto kEpochDurationSeconds{30};
  constexpr auto kBlockDelaySecs{kEpochDurationSeconds};
  constexpr auto kPropagationDelaySecs{6};
}  // namespace fc
