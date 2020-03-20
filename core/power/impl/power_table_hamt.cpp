/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power/impl/power_table_hamt.hpp"

#include "adt/address_key.hpp"
#include "power/power_table_error.hpp"

using fc::adt::AddressKey;
using fc::power::Power;
using fc::power::PowerTableError;
using fc::power::PowerTableHamt;
using fc::primitives::address::Address;
using fc::storage::hamt::HamtError;
using fc::storage::hamt::Value;

PowerTableHamt::PowerTableHamt(Hamt hamt) : power_table_{std::move(hamt)} {}

fc::outcome::result<Power> PowerTableHamt::getMinerPower(
    const Address &address) const {
  auto result = power_table_.getCbor<Power>(AddressKey::encode(address));
  if (!result && result.error() == HamtError::NOT_FOUND) {
    return outcome::failure(PowerTableError::NO_SUCH_MINER);
  }
  return result;
}

fc::outcome::result<void> PowerTableHamt::setMinerPower(const Address &address,
                                                        Power power_amount) {
  if (power_amount < 0) return PowerTableError::NEGATIVE_POWER;
  return power_table_.setCbor(AddressKey::encode(address), power_amount);
}

fc::outcome::result<void> PowerTableHamt::removeMiner(const Address &address) {
  auto result = power_table_.remove(AddressKey::encode(address));
  if (!result && result.error() == HamtError::NOT_FOUND) {
    return outcome::failure(PowerTableError::NO_SUCH_MINER);
  }
  return result;
}

fc::outcome::result<size_t> PowerTableHamt::getSize() const {
  size_t n = 0;
  Hamt::Visitor counter{[&n](auto k, auto v) {
    ++n;
    return fc::outcome::success();
  }};
  OUTCOME_TRY(power_table_.visit(counter));
  return n;
}

fc::outcome::result<Power> PowerTableHamt::getMaxPower() const {
  Power max = 0;
  Hamt::Visitor max_visitor{
      [&max](auto k, auto v) -> fc::outcome::result<void> {
        OUTCOME_TRY(power, codec::cbor::decode<Power>(v));
        if (max < power) max = power;
        return fc::outcome::success();
      }};
  OUTCOME_TRY(power_table_.visit(max_visitor));
  return max;
}

fc::outcome::result<std::vector<Address>> PowerTableHamt::getMiners() const {
  std::vector<Address> miners;
  Hamt::Visitor max_visitor{
      [&miners](auto k, auto v) -> fc::outcome::result<void> {
        OUTCOME_TRY(address, AddressKey::decode(k));
        miners.push_back(address);
        return fc::outcome::success();
      }};
  OUTCOME_TRY(power_table_.visit(max_visitor));
  return miners;
}
