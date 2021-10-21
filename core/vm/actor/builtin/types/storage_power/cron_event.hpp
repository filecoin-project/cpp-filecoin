/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::storage_power {
  using primitives::address::Address;

  struct CronEvent {
    Address miner_address;
    Bytes callback_payload;
  };
  CBOR_TUPLE(CronEvent, miner_address, callback_payload)

}  // namespace fc::vm::actor::builtin::types::storage_power
