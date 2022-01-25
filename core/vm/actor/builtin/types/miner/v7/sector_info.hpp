/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/sector_info.hpp"

namespace fc::vm::actor::builtin::v7::miner {

  struct SectorOnChainInfo : public v0::miner::SectorOnChainInfo {};
  CBOR_TUPLE(SectorOnChainInfo,
             sector,
             seal_proof,
             sealed_cid,
             deals,
             activation_epoch,
             expiration,
             deal_weight,
             verified_deal_weight,
             init_pledge,
             expected_day_reward,
             expected_storage_pledge,
             sector_key_cid)

}  // namespace fc::vm::actor::builtin::v7::miner
