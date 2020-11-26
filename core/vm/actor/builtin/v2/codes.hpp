/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V2_CODES_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V2_CODES_HPP

#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::v2 {

  const static CodeId kAccountCodeCid =
      CodeId(makeRawIdentityCid("fil/2/account"));
  const static CodeId kCronCodeCid = CodeId(makeRawIdentityCid("fil/2/cron"));
  const static CodeId kStoragePowerCodeCid =
      CodeId(makeRawIdentityCid("fil/2/storagepower"));
  const static CodeId kStorageMarketCodeCid =
      CodeId(makeRawIdentityCid("fil/2/storagemarket"));
  const static CodeId kStorageMinerCodeCid =
      CodeId(makeRawIdentityCid("fil/2/storageminer"));
  const static CodeId kMultisigCodeCid =
      CodeId(makeRawIdentityCid("fil/2/multisig"));
  const static CodeId kInitCodeCid = CodeId(makeRawIdentityCid("fil/2/init"));
  const static CodeId kPaymentChannelCodeCid =
      CodeId(makeRawIdentityCid("fil/2/paymentchannel"));
  const static CodeId kRewardActorCodeID =
      CodeId(makeRawIdentityCid("fil/2/reward"));
  const static CodeId kSystemActorCodeID =
      CodeId(makeRawIdentityCid("fil/2/system"));
  const static CodeId kVerifiedRegistryCode =
      CodeId(makeRawIdentityCid("fil/2/verifiedregistry"));

}  // namespace fc::vm::actor::builtin::v2

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V2_CODES_HPP
