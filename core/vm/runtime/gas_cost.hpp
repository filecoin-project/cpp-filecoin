/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_GAS_COST_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_GAS_COST_HPP

#include "primitives/types.hpp"

namespace fc::vm::runtime {
  using primitives::GasAmount;

  inline static const GasAmount kInfiniteGas{-1};

  // TODO (a.chernyshov) https://soramitsu.atlassian.net/browse/FIL-131 Assign
  // after spec is updated. All constants that are not defined in Lotus are
  // initialized with this value.
  inline static const GasAmount kGasAmountPlaceholder{0};

  inline static const GasAmount kOnChainReturnValuePerByteCost{8};

  /**
   * Gas cost charged to the originator of a non-nil return value produced by an
   * on-chain message is given by:
   *   len(return value)*OnChainReturnValuePerByte
   */
  inline static const GasAmount kOnChainReturnValuePerByteGasCost{
      kGasAmountPlaceholder};

  /**
   * Gas cost (Base + len * PerByte) for any Get operation to the IPLD store in
   * the runtime VM context.
   */
  inline static const GasAmount kIpldGetBaseGasCost{10};
  inline static const GasAmount kIpldGetPerByteGasCost{1};

  /**
   * Gas cost (Base + len * PerByte) for any Put operation to the IPLD store in
   * the runtime VM context.
   * Note: these costs should be significantly higher than the costs for Get
   * operations, since they reflect not only serialization/deserialization but
   * also persistent storage of chain data.
   */
  inline static const GasAmount kIpldPutBaseGasCost{20};
  inline static const GasAmount kIpldPutPerByteGasCost{2};

  /**
   * Gas cost for updating an actor's substate (i.e., UpdateRelease). This is in
   * addition to a per-byte fee for the state as for IPLD Get/Put.
   */
  inline static const GasAmount kUpdateActorSubstateGasCost{
      kGasAmountPlaceholder};

  inline static const GasAmount kCreateActorExtraGasCost{500};

  /**
   * Gas cost for creating a new actor (via InitActor's Exec method). Actor
   * sub-state is charged separately.
   */
  inline static const GasAmount kCreateActorGasCost{40
                                                    + kCreateActorExtraGasCost};

  /**
   * Gas cost for deleting an actor.
   */
  inline static const GasAmount kDeleteActorGasCost{-kCreateActorExtraGasCost};

  inline static const GasAmount kInitActorExecCost{100};
}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_GAS_COST_HPP
