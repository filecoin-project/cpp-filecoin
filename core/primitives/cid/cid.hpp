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
     * ContentIdentifier is not default-constructable, but in some cases we need
     * default value. This value can be used to initialize class member or local
     * variable. Trying to CBOR encode this value will yield error, to ensure
     * proper initialization.
     */
    CID();

    explicit CID(const ContentIdentifier &cid);

    explicit CID(ContentIdentifier &&cid) noexcept;

    CID(CID &&cid) noexcept;

    CID(const CID &cid) = default;

    CID(Version version,
        libp2p::multi::MulticodecType::Code content_type,
        libp2p::multi::Multihash content_address);

    ~CID() = default;

    CID &operator=(const CID &) = default;

    CID &operator=(CID &&cid) noexcept;

    CID &operator=(const ContentIdentifier &cid);

    CID &operator=(ContentIdentifier &&cid);

    /**
     * @brief string-encodes cid
     * @return encoded value or error
     */
    outcome::result<std::string> toString() const;

    /**
     * @brief encodes CID to bytes
     * @return byte-representation of CID
     */
    outcome::result<std::vector<uint8_t>> toBytes() const;

    static outcome::result<CID> fromString(const std::string &str);
  };
}  // namespace fc

namespace fc::common {
  /// Compute CID from bytes
  outcome::result<CID> getCidOf(gsl::span<const uint8_t> bytes);

}  // namespace fc::common

#endif  // CPP_FILECOIN_CORE_COMMON_CID_HPP
