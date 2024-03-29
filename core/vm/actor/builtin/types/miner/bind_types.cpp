/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/types/miner/v0/expiration.hpp"
#include "vm/actor/builtin/types/miner/v2/expiration.hpp"
#include "vm/actor/builtin/types/miner/v3/expiration.hpp"

#include "vm/actor/builtin/types/miner/v0/miner_info.hpp"
#include "vm/actor/builtin/types/miner/v2/miner_info.hpp"
#include "vm/actor/builtin/types/miner/v3/miner_info.hpp"

#include "vm/actor/builtin/types/miner/v0/partition.hpp"
#include "vm/actor/builtin/types/miner/v2/partition.hpp"
#include "vm/actor/builtin/types/miner/v3/partition.hpp"

#include "vm/actor/builtin/types/miner/v0/monies.hpp"
#include "vm/actor/builtin/types/miner/v2/monies.hpp"
#include "vm/actor/builtin/types/miner/v3/monies.hpp"

#include "vm/actor/builtin/types/miner/v0/deadline.hpp"
#include "vm/actor/builtin/types/miner/v2/deadline.hpp"
#include "vm/actor/builtin/types/miner/v3/deadline.hpp"

#include "vm/actor/builtin/types/miner/v0/proof_policy.hpp"
#include "vm/actor/builtin/types/miner/v2/proof_policy.hpp"
#include "vm/actor/builtin/types/miner/v3/proof_policy.hpp"

#include "vm/actor/builtin/types/miner/v0/sector_info.hpp"
#include "vm/actor/builtin/types/miner/v7/sector_info.hpp"

UNIVERSAL_IMPL(miner::MinerInfo,
               v0::miner::MinerInfo,
               v2::miner::MinerInfo,
               v3::miner::MinerInfo,
               v3::miner::MinerInfo,
               v3::miner::MinerInfo,
               v3::miner::MinerInfo,
               v3::miner::MinerInfo)

UNIVERSAL_IMPL(miner::Partition,
               v0::miner::Partition,
               v2::miner::Partition,
               v3::miner::Partition,
               v3::miner::Partition,
               v3::miner::Partition,
               v3::miner::Partition,
               v3::miner::Partition)

UNIVERSAL_IMPL(miner::ExpirationQueue,
               v0::miner::ExpirationQueue,
               v2::miner::ExpirationQueue,
               v3::miner::ExpirationQueue,
               v3::miner::ExpirationQueue,
               v3::miner::ExpirationQueue,
               v3::miner::ExpirationQueue,
               v3::miner::ExpirationQueue)

UNIVERSAL_IMPL(miner::Monies,
               v0::miner::Monies,
               v2::miner::Monies,
               v3::miner::Monies,
               v3::miner::Monies,
               v3::miner::Monies,
               v3::miner::Monies,
               v3::miner::Monies)

UNIVERSAL_IMPL(miner::Deadline,
               v0::miner::Deadline,
               v2::miner::Deadline,
               v3::miner::Deadline,
               v3::miner::Deadline,
               v3::miner::Deadline,
               v3::miner::Deadline,
               v3::miner::Deadline)

UNIVERSAL_IMPL(miner::ProofPolicy,
               v0::miner::ProofPolicy,
               v2::miner::ProofPolicy,
               v3::miner::ProofPolicy,
               v3::miner::ProofPolicy,
               v3::miner::ProofPolicy,
               v3::miner::ProofPolicy,
               v3::miner::ProofPolicy)

UNIVERSAL_IMPL(miner::SectorOnChainInfo,
               v0::miner::SectorOnChainInfo,
               v0::miner::SectorOnChainInfo,
               v0::miner::SectorOnChainInfo,
               v0::miner::SectorOnChainInfo,
               v0::miner::SectorOnChainInfo,
               v0::miner::SectorOnChainInfo,
               v7::miner::SectorOnChainInfo)
