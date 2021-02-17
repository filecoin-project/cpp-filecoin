/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/actor.hpp"

#pragma once

namespace fc::vm::toolchain {
  using actor::CodeId;

  /** Address matcher for all actor versions */
  class CommonAddressMatcher {
   public:
    /** Checks if code is an account actor */
    static bool isAccountActor(const CodeId &code);

    /** Checks if code is cron actor */
    static bool isCronActor(const CodeId &code);

    /** Checks if code is power actor */
    static bool isStoragePowerActor(const CodeId &code);

    /** Checks if code is market actor */
    static bool isStorageMarketActor(const CodeId &code);

    /** Checks if code is miner actor */
    static bool isStorageMinerActor(const CodeId &code);

    /** Checks if code is multisig actor */
    static bool isMultisigActor(const CodeId &code);

    /** Checks if code is init actor */
    static bool isInitActor(const CodeId &code);

    /** Checks if code is payment channel actor */
    static bool isPaymentChannelActor(const CodeId &code);

    /** Checks if code is reward actor */
    static bool isRewardActor(const CodeId &code);

    /** Checks if code is system actor */
    static bool isSystemActor(const CodeId &code);

    /** Checks if code is verified registry actor */
    static bool isVerifiedRegistryActor(const CodeId &code);
  };

}  // namespace fc::vm::toolchain
