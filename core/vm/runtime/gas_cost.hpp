/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_GAS_COST_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_GAS_COST_HPP

#include "primitives/big_int.hpp"

namespace fc::vm::runtime {

  using primitives::BigInt;

  inline static const BigInt kInfiniteGas{-1};

  // TODO (a.chernyshov) https://soramitsu.atlassian.net/browse/FIL-131 Assign
  // after spec is updated. All constants that are not defined in Lotus are
  // initialized with this value.
  inline static const BigInt kGasAmountPlaceholder{0};

  /**
   * Gas cost charged to the originator of an on-chain message (regardless of
   * whether it succeeds or fails in application) is given by:
   *   OnChainMessageBase + len(serialized message) * OnChainMessagePerByte
   * Together, these account for the cost of message propagation and validation,
   * up to but excluding any actual processing by the VM.
   * This is the cost a block producer burns when including an invalid message.
   */
  inline static const BigInt kOnChainMessageBaseGasCost{kGasAmountPlaceholder};
  inline static const BigInt kOnChainMessagePerByteGasCharge{2};

  /**
   * Gas cost charged to the originator of a non-nil return value produced by an
   * on-chain message is given by:
   *   len(return value)*OnChainReturnValuePerByte
   */
  inline static const BigInt kOnChainReturnValuePerByteGasCost{
      kGasAmountPlaceholder};

  /**
   * Gas cost for any message send execution (including the top-level one
   * initiated by an on-chain message). This accounts for the cost of loading
   * sender and receiver actors and (for top-level messages) incrementing the
   * sender's sequence number. Load and store of actor sub-state is charged
   * separately.
   */
  inline static const BigInt kSendBaseGasCost{kGasAmountPlaceholder};

  /**
   * Gas cost charged, in addition to SendBase, if a message send is accompanied
   * by any nonzero currency amount. Accounts for writing receiver's new balance
   * (the sender's state is already accounted for).
   */
  inline static const BigInt kSendTransferFundsGasCost{10};

  /**
   * Gas cost charged, in addition to SendBase, if a message invokes a method on
   * the receiver. Accounts for the cost of loading receiver code and method
   * dispatch.
   */
  inline static const BigInt kSendInvokeMethodGasCost{5};

  /**
   * Gas cost (Base + len * PerByte) for any Get operation to the IPLD store in
   * the runtime VM context.
   */
  inline static const BigInt kIpldGetBaseGasCost{10};
  inline static const BigInt kIpldGetPerByteGasCost{1};

  /**
   * Gas cost (Base + len * PerByte) for any Put operation to the IPLD store in
   * the runtime VM context.
   * Note: these costs should be significantly higher than the costs for Get
   * operations, since they reflect not only serialization/deserialization but
   * also persistent storage of chain data.
   */
  inline static const BigInt kIpldPutBaseGasCost{20};
  inline static const BigInt kIpldPutPerByteGasCost{2};

  /**
   * Gas cost for updating an actor's substate (i.e., UpdateRelease). This is in
   * addition to a per-byte fee for the state as for IPLD Get/Put.
   */
  inline static const BigInt kUpdateActorSubstateGasCost{kGasAmountPlaceholder};

  /**
   * Gas cost for creating a new actor (via InitActor's Exec method). Actor
   * sub-state is charged separately.
   */
  inline static const BigInt kExecNewActorGasCost{kGasAmountPlaceholder};

  /**
   * Gas cost for deleting an actor.
   */
  inline static const BigInt kDeleteActorGasCost{kGasAmountPlaceholder};

  /**
   * Gas cost charged per public-key cryptography operation (e.g., signature
   * verification).
   */
  inline static const BigInt kPublicKeyCryptoOperationGasCost{
      kGasAmountPlaceholder};

  /**
   * Gas cost of new state commit
   */
  inline static const BigInt kCommitGasCost{50};

  inline static const BigInt kInitActorExecCost{100};
}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_GAS_COST_HPP
