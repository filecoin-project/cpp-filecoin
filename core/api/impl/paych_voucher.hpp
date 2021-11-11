/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "paych/vouchers.hpp"

namespace fc::api {
  inline void implPaychVoucher(
      const std::shared_ptr<FullNodeApi> &api,
      const std::shared_ptr<paych_vouchers::PaychVouchers> &vouchers) {
    using paych_vouchers::actorHash;
    api->PaychAllocateLane =
        [=](const Address &address) -> outcome::result<LaneId> {
      OUTCOME_TRY(paych, actorHash(address));
      return vouchers->nextLane(paych);
    };
    api->PaychVoucherAdd =
        [=](const Address &address,
            const SignedVoucher &voucher,
            const Bytes &proof,
            const TokenAmount &min_delta) -> outcome::result<TokenAmount> {
      if (!proof.empty()) {
        return ERROR_TEXT("PaychVoucherAdd proof not supported");
      }
      OUTCOME_TRY(paych, actorHash(address));
      OUTCOME_TRY(voucher_paych, actorHash(voucher.channel));
      if (paych != voucher_paych) {
        return ERROR_TEXT("PaychVoucherAdd wrong address");
      }
      return vouchers->add(voucher, min_delta);
    };
    api->PaychVoucherCheckValid =
        [=](const Address &address,
            const SignedVoucher &voucher) -> outcome::result<void> {
      OUTCOME_TRY(paych, actorHash(address));
      OUTCOME_TRY(voucher_paych, actorHash(voucher.channel));
      if (paych != voucher_paych) {
        return ERROR_TEXT("PaychVoucherCheckValid wrong address");
      }
      return vouchers->check(voucher);
    };
    api->PaychVoucherCreate =
        [=](const Address &address,
            const TokenAmount &amount,
            LaneId lane) -> outcome::result<SignedVoucher> {
      OUTCOME_TRY(paych, actorHash(address));
      return vouchers->make(paych, lane, amount);
    };
  }
}  // namespace fc::api
