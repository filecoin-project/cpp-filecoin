

#pragma once
#include "common/outcome.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "const.hpp"
#include "primitives/types.hpp"
#include "vm/version/version.hpp"
#include "core/vm/actor/builtin/types/miner/policy.hpp"

namespace vm::actor::builtin::v3::miner {
  using fc::common::smoothing::FilterEstimate;
  using fc::primitives::ChainEpoch;
  using fc::primitives::StoragePower;
  using fc::primitives::TokenAmount;
  using fc::vm::version::NetworkVersion;
  using fc::vm::actor::builtin::types::miner::VestSpec;

  class Monies : public vm::actor::builtin::v2::miner::Monies {
   protected:
    fc::primitives::BigInt locked_reward_factor_num = fc::primitives::BigInt(75);
    fc::primitives::BigInt locked_reward_factor_denom = fc::primitives::BigInt(100);

    fc::primitives::BigInt base_reward_for_disputed_window_po_st = fc::primitives::BigInt(4) * /* TODO token_precision */;
    fc::primitives::BigInt base_penalty_for_disputed_window_po_st = fc::primitives::BigInt(20) * /* TODO token_precision */;

   private:
/*
    virtual fc::outcome::result < std::pair < TokenAmount,
        ? ? ? >> (const TokenAmount &reward, const fc::vm::version &nv);
        */ // TODO
  };

}  // namespace vm::actor::builtin::v3::miner