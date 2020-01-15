/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_COMMON_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_COMMON_HPP

#include <cstdint>

#include <libp2p/multi/content_identifier_codec.hpp>

#include "codec/cbor/cbor_errors.hpp"
#include "common/outcome_throw.hpp"

namespace fc::codec::cbor {
  constexpr uint64_t kCidTag = 42;

  constexpr uint64_t kMaxLength = 8192u;
  constexpr uint64_t kByteArrayMaxLength = 2u << 20u;

  /*
   * const (
	MajUnsignedInt = 0
	MajNegativeInt = 1
	MajByteString  = 2
	MajTextString  = 3
	MajArray       = 4
	MajMap         = 5
	MajTag         = 6
	MajOther       = 7
)
   */
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_COMMON_HPP
