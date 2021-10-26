/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/types/market/v0/pending_proposals.hpp"
#include "vm/actor/builtin/types/market/v2/pending_proposals.hpp"
#include "vm/actor/builtin/types/market/v3/pending_proposals.hpp"

UNIVERSAL_IMPL(market::PendingProposals,
               v0::market::PendingProposals,
               v2::market::PendingProposals,
               v3::market::PendingProposals,
               v3::market::PendingProposals,
               v3::market::PendingProposals,
               v3::market::PendingProposals)
