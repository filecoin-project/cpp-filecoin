/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin {

  // These methods must be actual with the last version of actors

  enum class VerifiedRegistryActor : MethodNumber {
    kConstruct = 1,
    kAddVerifier,
    kRemoveVerifier,
    kAddVerifiedClient,
    kUseBytes,
    kRestoreBytes,
    kRemoveVerifiedClientDataCap,  // since v7
  }

}  // namespace fc::vm::actor::builtin
