/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_API_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_API_HPP

#define API_METHOD(_name, _result, ...)                                    \
  struct _##_name : std::function<outcome::result<_result>(__VA_ARGS__)> { \
    using function::function;                                              \
    using Result = _result;                                                \
    using Params = ParamsTuple<__VA_ARGS__>;                               \
    static constexpr auto name = "FuhonMiner." #_name;                     \
  } _name;

#include <vm/actor/builtin/miner/types.hpp>
#include "common/buffer.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::mining {
  using common::Buffer;
  using primitives::address::Address;
  using TipsetToken = Buffer;
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;
  using primitives::ChainEpoch;
  using primitives::SectorNumber;
  using primitives::TokenAmount;
  using vm::VMExitCode;
  using vm::actor::MethodNumber;
  using vm::actor::builtin::miner::SectorPreCommitOnChainInfo;

  template <typename... T>
  using ParamsTuple =
      std::tuple<std::remove_const_t<std::remove_reference_t<T>>...>;

  class SealingApi {
   public:
    struct ChainHeadResponse {
      TipsetToken tipset;
      ChainEpoch epoch;
    };

    API_METHOD(ChainHead, ChainHeadResponse);

    API_METHOD(ChainGetRandomness,
               Randomness,
               const TipsetToken &,
               DomainSeparationTag,
               ChainEpoch,
               const Buffer &)

    API_METHOD(StateSectorPreCommitInfo,
               SectorPreCommitOnChainInfo,
               const Address &,
               SectorNumber,
               const TipsetToken &);

    API_METHOD(StateMinerWorkerAddress,
               Address,
               const Address &,
               const TipsetToken &);

    struct MessageReceipt {
      VMExitCode exit_code;
      Buffer return_value;
      int64_t gas_used;
    };

    struct MessageLookup {
      MessageReceipt receipt;
      TipsetToken tipset_token;
      ChainEpoch height;
    };

    API_METHOD(StateWaitMsg, MessageLookup, CID);

    API_METHOD(SendMsg,
               CID,
               const Address &,
               const Address &,
               MethodNumber,
               TokenAmount,
               TokenAmount,
               int64_t,
               const Buffer &);
  };

}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_API_HPP
