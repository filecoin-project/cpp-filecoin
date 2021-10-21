/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/endian.hpp>
#include "common/bytes.hpp"

namespace fc::common {
  inline void putUint64BigEndian(Bytes &l, uint64_t n) {
    l.resize(l.size() + sizeof(n));
    boost::endian::store_big_u64(&(*(l.end() - sizeof(n))), n);
  }
}  // namespace fc::common
