/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_COMMON_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_COMMON_HPP

#include "codec/cbor/cbor_errors.hpp"
#include "common/outcome_throw.hpp"
#include <cstdint>
#include <libp2p/multi/content_identifier_codec.hpp>

namespace fc::codec::cbor {
  constexpr uint64_t kCidTag = 42;
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_COMMON_HPP
