/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_OUTCOME_HPP
#define CPP_FILECOIN_CORE_COMMON_OUTCOME_HPP

#include <libp2p/outcome/outcome.hpp>

namespace fc::outcome {
  using libp2p::outcome::failure;
  using libp2p::outcome::result;
  using libp2p::outcome::success;
}  // namespace fc::outcome

#endif // CPP_FILECOIN_CORE_COMMON_OUTCOME_HPP
