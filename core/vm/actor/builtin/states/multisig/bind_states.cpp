/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/states/multisig/multisig_actor_state.hpp"
#include "vm/actor/builtin/states/multisig/v0/multisig_actor_state.hpp"
#include "vm/actor/builtin/states/multisig/v2/multisig_actor_state.hpp"
#include "vm/actor/builtin/states/multisig/v3/multisig_actor_state.hpp"

UNIVERSAL_IMPL(states::MultisigActorState,
               v0::multisig::MultisigActorState,
               v2::multisig::MultisigActorState,
               v3::multisig::MultisigActorState,
               v3::multisig::MultisigActorState,
               v3::multisig::MultisigActorState,
               v3::multisig::MultisigActorState)
