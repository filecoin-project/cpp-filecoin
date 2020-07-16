/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_LIBP2P_CBOR_BUFFERING_HPP
#define CPP_FILECOIN_CORE_COMMON_LIBP2P_CBOR_BUFFERING_HPP

#include <vector>

#include <boost/optional.hpp>
#include <gsl/span>

#include "common/outcome.hpp"

namespace fc::common::libp2p {
  /// Incrementally decodes length of cbor object bytes
  struct CborBuffering {
    enum class Error {
      kInvalidHeadType = 1,
      kInvalidHeadValue,
    };

    enum class Type {
      Unsigned = 0,
      Signed = 1,
      Bytes = 2,
      Text = 3,
      Array = 4,
      Map = 5,
      Tag = 6,
      Special = 7,
    };

    struct Head {
      /// Parse first header byte
      static outcome::result<Head> first(size_t &more, uint8_t first);

      /// Parse next header byte
      static void next(size_t &more, uint8_t next, Head &head);

      Type type;
      uint64_t value;
    };

    /// Was object fully read
    bool done() const;

    /// Reset state to read next object
    void reset();

    /// How many bytes are required to continue reading
    size_t moreBytes() const;

    /// Continue reading, return bytes consumed
    outcome::result<size_t> consume(gsl::span<const uint8_t> input);

    std::vector<size_t> more_nested;
    size_t more_bytes{};
    boost::optional<Head> partial_head;
  };
}  // namespace fc::common::libp2p

OUTCOME_HPP_DECLARE_ERROR(fc::common::libp2p, CborBuffering::Error)

#endif  // CPP_FILECOIN_CORE_COMMON_LIBP2P_CBOR_BUFFERING_HPP
