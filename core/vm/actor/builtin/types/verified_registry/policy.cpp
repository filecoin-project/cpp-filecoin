/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/verified_registry/policy.hpp"

namespace fc::vm::actor::builtin::types::verified_registry {

  StoragePower kMinVerifiedDealSize{StoragePower{1} << 20};

  void setPolicy(const StoragePower &minVerifiedDealSize) {
    kMinVerifiedDealSize = minVerifiedDealSize;
  }
}
