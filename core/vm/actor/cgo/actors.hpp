/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "fwd.hpp"

namespace fc::vm::actor::cgo {
  using runtime::Runtime;

  void configMainnet();

  outcome::result<Buffer> invoke(const CID &code,
                                 const std::shared_ptr<Runtime> &runtime);
}  // namespace fc::vm::actor::cgo
