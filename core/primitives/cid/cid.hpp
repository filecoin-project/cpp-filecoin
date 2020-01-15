/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_CID_HPP
#define CPP_FILECOIN_CORE_COMMON_CID_HPP

#include <libp2p/multi/content_identifier.hpp>

#include "common/outcome.hpp"

namespace fc {
  class CID : public libp2p::multi::ContentIdentifier {
   public:
    using ContentIdentifier::ContentIdentifier;

    /**
     * ContentIdentifier is not default-constructible, but in some cases we need
     * default value. This value can be used to initialize class member or local
     * variable. Trying to CBOR encode this value will yield error, to ensure
     * proper initialization.
     */
    CID();
    CID(const ContentIdentifier &cid);
  };
}  // namespace fc

namespace fc::common {
  /// Compute CID from bytes
  outcome::result<CID> getCidOf(gsl::span<const uint8_t> bytes);
}  // namespace fc::common

#endif  // CPP_FILECOIN_CORE_COMMON_CID_HPP
