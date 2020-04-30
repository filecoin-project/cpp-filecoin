/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_COMMON_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_COMMON_HPP

#include <cstdint>

#include <boost/optional.hpp>

#include "codec/cbor/cbor_errors.hpp"
#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::codec::cbor {
  constexpr uint64_t kCidTag = 42;
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_COMMON_HPP
