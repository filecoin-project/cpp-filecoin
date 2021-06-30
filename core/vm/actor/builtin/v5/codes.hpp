/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::v5 {
  const static CodeId kAccountCodeId =
      CodeId(makeRawIdentityCid("fil/5/account"));
  const static CodeId kCronCodeId = CodeId(makeRawIdentityCid("fil/5/cron"));
  const static CodeId kStoragePowerCodeId =
      CodeId(makeRawIdentityCid("fil/5/storagepower"));
  const static CodeId kStorageMarketCodeId =
      CodeId(makeRawIdentityCid("fil/5/storagemarket"));
  const static CodeId kStorageMinerCodeId =
      CodeId(makeRawIdentityCid("fil/5/storageminer"));
  const static CodeId kMultisigCodeId =
      CodeId(makeRawIdentityCid("fil/5/multisig"));
  const static CodeId kInitCodeId = CodeId(makeRawIdentityCid("fil/5/init"));
  const static CodeId kPaymentChannelCodeId =
      CodeId(makeRawIdentityCid("fil/5/paymentchannel"));
  const static CodeId kRewardActorCodeId =
      CodeId(makeRawIdentityCid("fil/5/reward"));
  const static CodeId kSystemActorCodeId =
      CodeId(makeRawIdentityCid("fil/5/system"));
  const static CodeId kVerifiedRegistryCodeId =
      CodeId(makeRawIdentityCid("fil/5/verifiedregistry"));
}  // namespace fc::vm::actor::builtin::v5
