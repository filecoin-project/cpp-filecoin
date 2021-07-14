/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc::vm::actor {
  enum class ActorVersion {
    kVersion0 = 0,
    kVersion2 = 2,
    kVersion3 = 3,
    kVersion4 = 4,
    kVersion5 = 5,
  };

  struct WithActorVersion {
    ActorVersion actor_version{};

    WithActorVersion() = default;
    explicit WithActorVersion(ActorVersion v) : actor_version{v} {}

   protected:
    ~WithActorVersion() = default;
  };
}  // namespace fc::vm::actor
