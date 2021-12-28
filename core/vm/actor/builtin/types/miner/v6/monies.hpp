/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc::vm::actor::builtin::v6::miner {
  using primitives::BigInt;

  const BigInt kEstimatedSingleProveCommitGasUsage = 49299973;
  const BigInt kEstimatedSinglePreCommitGasUsage = 16433324;
  const BigInt kBatchDiscountNumerator = 1;
  const BigInt kBatchDiscountDenominator = 20;
  const BigInt kBatchBalancer = 5 * kOneNanoFil;

  TokenAmount aggregatePreCommitNetworkFee(uint64_t aggregate_size,
                                           const TokenAmount &base_fee) {
    const TokenAmount effectiveGasFee = std::max(base_fee, kBatchBalancer);
    const TokenAmount networkFeeNum =
        effectiveGasFee * kEstimatedSinglePreCommitGasUsage * aggregate_size
        * kBatchDiscountNumerator;
    const TokenAmount networkFee =
        bigdiv(networkFeeNum, kBatchDiscountDenominator);
    return networkFee;
  }

}  // namespace fc::vm::actor::builtin::v6::miner
