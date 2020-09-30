/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "common/outcome2.hpp"

namespace fc::common {
  Outcome<Buffer> readFile(std::string_view path);

  outcome::result<void> writeFile(std::string_view path, BytesIn input);
}  // namespace fc::common
