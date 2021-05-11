/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

#include <boost/optional.hpp>

#include "codec/cbor/cbor_errors.hpp"
#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::codec::cbor {
  constexpr uint64_t kCidTag = 42;
}  // namespace fc::codec::cbor
