/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <memory>
#include <mutex>

namespace fc::mining {
  class PreCommitBatcher {
   public:

    virtual ~PreCommitBatcher() = default;
  };

}  // namespace fc::mining