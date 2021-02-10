/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::v0 {

  const static CodeId kAccountCodeId =
      CodeId(makeRawIdentityCid("fil/1/account"));
  const static CodeId kCronCodeId = CodeId(makeRawIdentityCid("fil/1/cron"));
  const static CodeId kStoragePowerCodeId =
      CodeId(makeRawIdentityCid("fil/1/storagepower"));
  const static CodeId kStorageMarketCodeId =
      CodeId(makeRawIdentityCid("fil/1/storagemarket"));
  const static CodeId kStorageMinerCodeId =
      CodeId(makeRawIdentityCid("fil/1/storageminer"));
  const static CodeId kMultisigCodeId =
      CodeId(makeRawIdentityCid("fil/1/multisig"));
  const static CodeId kInitCodeId = CodeId(makeRawIdentityCid("fil/1/init"));
  const static CodeId kPaymentChannelCodeCid =
      CodeId(makeRawIdentityCid("fil/1/paymentchannel"));
  const static CodeId kRewardActorCodeId =
      CodeId(makeRawIdentityCid("fil/1/reward"));
  const static CodeId kSystemActorCodeId =
      CodeId(makeRawIdentityCid("fil/1/system"));
  const static CodeId kVerifiedRegistryCodeId =
      CodeId(makeRawIdentityCid("fil/1/verifiedregistry"));

}  // namespace fc::vm::actor::builtin::v0
