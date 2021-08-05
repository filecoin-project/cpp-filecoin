/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/storage_power/claim.hpp"

#include "codec/cbor/streams_annotation.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {

  struct Claim : types::storage_power::Claim {};
  CBOR_TUPLE(Claim, raw_power, qa_power)

}  // namespace fc::vm::actor::builtin::v0::storage_power
