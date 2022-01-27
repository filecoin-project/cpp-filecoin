/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/version/version.hpp"

namespace fc::vm::actor {
  enum class ActorVersion {
    kVersion0 = 0,
    kVersion2 = 2,
    kVersion3 = 3,
    kVersion4 = 4,
    kVersion5 = 5,
    kVersion6 = 6,
    kVersion7 = 7,
  };

  struct WithActorVersion {
    ActorVersion actor_version{};

    WithActorVersion() = default;
    explicit WithActorVersion(ActorVersion v) : actor_version{v} {}

   protected:
    ~WithActorVersion() = default;
  };
}  // namespace fc::vm::actor

namespace fc {
  using primitives::ChainEpoch;
  using vm::actor::ActorVersion;
  using vm::version::NetworkVersion;

  inline ActorVersion actorVersion(NetworkVersion version) {
    switch (version) {
      case NetworkVersion::kVersion0:
      case NetworkVersion::kVersion1:
      case NetworkVersion::kVersion2:
      case NetworkVersion::kVersion3:
        return ActorVersion::kVersion0;
      case NetworkVersion::kVersion4:
      case NetworkVersion::kVersion5:
      case NetworkVersion::kVersion6:
      case NetworkVersion::kVersion7:
      case NetworkVersion::kVersion8:
      case NetworkVersion::kVersion9:
        return ActorVersion::kVersion2;
      case NetworkVersion::kVersion10:
      case NetworkVersion::kVersion11:
        return ActorVersion::kVersion3;
      case NetworkVersion::kVersion12:
        return ActorVersion::kVersion4;
      case NetworkVersion::kVersion13:
        return ActorVersion::kVersion5;
      case NetworkVersion::kVersion14:
        return ActorVersion::kVersion6;
      case NetworkVersion::kVersion15:
        return ActorVersion::kVersion7;
    }
  }
  inline ActorVersion actorVersion(ChainEpoch epoch) {
    return actorVersion(vm::version::getNetworkVersion(epoch));
  }
}  // namespace fc
