/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_CID_HPP
#define CPP_FILECOIN_CORE_COMMON_CID_HPP

#include <libp2p/multi/content_identifier.hpp>

#include "common/outcome.hpp"

namespace fc {
  using CID = libp2p::multi::ContentIdentifier;
}  // namespace fc

namespace fc::common {
  /**
   * CID is not default-constructible, but in some cases we need default value.
   * This value can be used to initialize class member or local variable. Trying
   * to CBOR encode this value will yield error, to ensure proper
   * initialization.
   */
  const static inline CID kEmptyCid(
      {}, {}, libp2p::multi::Multihash::create({}, {}).value());

  /// Compute CID from bytes
  outcome::result<CID> getCidOf(gsl::span<const uint8_t> bytes);
}  // namespace fc::common

#endif  // CPP_FILECOIN_CORE_COMMON_CID_HPP
