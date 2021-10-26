/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/types/storage_power/claim.hpp"
#include "vm/actor/builtin/types/storage_power/v0/claim.hpp"
#include "vm/actor/builtin/types/storage_power/v2/claim.hpp"
#include "vm/actor/builtin/types/storage_power/v3/claim.hpp"

UNIVERSAL_IMPL(storage_power::Claim,
               v0::storage_power::Claim,
               v2::storage_power::Claim,
               v3::storage_power::Claim,
               v3::storage_power::Claim,
               v3::storage_power::Claim,
               v3::storage_power::Claim)
