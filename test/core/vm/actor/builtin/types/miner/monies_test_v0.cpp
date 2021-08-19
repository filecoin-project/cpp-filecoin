#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/v0/monies.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  class MoniesTestv0 : public testing::Test {
   protected:
    virtual void SetUp(){
      nv = vm::version::NetworkVersion::kVersion0;
      epoch_target_reward = TokenAmount{1 << 50};
      qa_sector_power = StoragePower{1 << 36};
      network_qa_power = StoragePower{1 << 50};

      reward_estimate = {.position = epoch_target_reward};
      power_estimate = {.position = network_qa_power};

      EXPECT_OUTCOME_TRUE(
          maybe_penalty,
          moniesv0.PledgePenaltyForUndeclaredFault(
              reward_estimate, &power_estimate, qa_sector_power, nv));
      undeclared_penalty_ = TokenAmount{maybe_penalty};
      std::cout<<undeclared_penalty_<<"\n";
      big_initial_pledge_factor = 20;
    }

    Monies moniesv0;

    vm::version::NetworkVersion nv;

    TokenAmount epoch_target_reward;
    StoragePower qa_sector_power;
    StoragePower network_qa_power;

    FilterEstimate reward_estimate;
    FilterEstimate power_estimate;

    TokenAmount undeclared_penalty_;
    BigInt big_initial_pledge_factor;
  };

  TEST_F(MoniesTestv0, TestPledgePenaltyForTermination) {
    TokenAmount initial_pledge = TokenAmount{1 << 10};
    TokenAmount day_reward =
        initial_pledge / big_initial_pledge_factor;
    TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;
    ChainEpoch sector_age = 20 * ChainEpoch{static_cast<BigInt>(kEpochsInDay)};

    EXPECT_OUTCOME_EQ(
                        moniesv0.PledgePenaltyForTermination(day_reward,
                                                             twenty_day_reward,
                                                             sector_age,
                                                             reward_estimate,
                                                             &power_estimate,
                                                             qa_sector_power,
                                                             nv), undeclared_penalty_)
  }

  TEST_F(MoniesTestv0, ExpectedRewardFault) {
    TokenAmount initial_pledge = undeclared_penalty_;
    TokenAmount day_reward =
        initial_pledge / big_initial_pledge_factor;
    TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;
    int64_t sector_age_in_days = 20;
    ChainEpoch sector_age = ChainEpoch{
        static_cast<ChainEpoch>(sector_age_in_days * fc::kEpochsInDay)};

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv0.PledgePenaltyForTermination(day_reward,
                                                             twenty_day_reward,
                                                             sector_age,
                                                             reward_estimate,
                                                             &power_estimate,
                                                             qa_sector_power,
                                                             nv))

    TokenAmount expectedFee =
        initial_pledge
        + initial_pledge * sector_age_in_days / big_initial_pledge_factor;

    ASSERT_EQ(expectedFee, fee);
  }

  TEST_F(MoniesTestv0, CappedSectorAge) {
    TokenAmount initial_pledge = undeclared_penalty_;
    TokenAmount day_reward =
        initial_pledge / big_initial_pledge_factor;
    TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;
    int64_t sector_age_in_days = 500;
    ChainEpoch sector_age = ChainEpoch{
        static_cast<ChainEpoch>(sector_age_in_days * fc::kEpochsInDay)};

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv0.PledgePenaltyForTermination(day_reward,
                                                             twenty_day_reward,
                                                             sector_age,
                                                             reward_estimate,
                                                             &power_estimate,
                                                             qa_sector_power,
                                                             nv))

    TokenAmount expected_fee = initial_pledge
                        + initial_pledge * moniesv0.termination_lifetime_cap
                              / big_initial_pledge_factor;
    ASSERT_EQ(expected_fee, fee);
  }
}  // namespace fc::vm::actor::builtin::v0::miner