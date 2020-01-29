/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/randomness/impl/randomness_provider_imp.hpp"

#include <gtest/gtest.h>

#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <unordered_set>

#include "testutil/crypto/entropy.hpp"
#include "testutil/literals.hpp"

using namespace fc::crypto::randomness;

using fc::common::Buffer;
using fc::common::Hash256;
using libp2p::crypto::random::BoostRandomGenerator;
using libp2p::crypto::random::CSPRNG;

struct RandomnessProviderBaseTest {
  std::shared_ptr<RandomnessProvider> randomness_provider =
      std::make_shared<RandomnessProviderImpl>();
};

struct RandomnessProviderTest
    : public RandomnessProviderBaseTest,
      public ::testing::TestWithParam<std::tuple<DomainSeparationTag,
                                                 Serialization,
                                                 ChainEpoch,
                                                 Randomness>> {
  template <class T, class S, class I, class R>
  static std::tuple<DomainSeparationTag, Serialization, ChainEpoch, Randomness>
  makeParams(T tag, S s, I index, R res) {
    return std::make_tuple(static_cast<DomainSeparationTag>(std::move(tag)),
                           static_cast<Serialization>(std::move(s)),
                           static_cast<ChainEpoch>(std::move(index)),
                           static_cast<Randomness>(std::move(res)));
  }
};

/**
 * @given randomness provider, set of tuples each containing DST, serialized
 * value, epoch index and corresponding randomness
 * @when deriveRandomness method of randomness provider is called taking first 3
 * params
 * @then the resulting randomness is equal to predefined randomness value
 */
TEST_P(RandomnessProviderTest, DeriveRandomnessSuccess) {
  auto &&[tag, s, index, result] = GetParam();
  auto &&res = randomness_provider->deriveRandomness(tag, s, index);
  std::cout << res << std::endl;
  ASSERT_EQ(res, result) << res;
}

/**
 * @given randomness provider, set of tuples each containing DST, serialized
 * value, epoch index and corresponding randomness
 * @when deriveRandomness method of randomness provider is called taking first 2
 * args
 * @then the result is equal to the result of calling randomness provider,
 * taking the same 2 args and -1 as index
 */
TEST_P(RandomnessProviderTest, TwoParamsMethodSuccess) {
  auto &&[tag, s, index, result] = GetParam();
  auto &&res1 = randomness_provider->deriveRandomness(tag, s);
  auto &&res2 = randomness_provider->deriveRandomness(tag, s, ChainEpoch(-1));
  ASSERT_EQ(res1, res2);
}

INSTANTIATE_TEST_CASE_P(
    RandomnessTestCases,
    RandomnessProviderTest,
    ::testing::Values(
        RandomnessProviderTest::makeParams(
            DomainSeparationTag::TicketDrawingDST,
            Buffer{1, 2, 3},
            1,
            "C4E51A115F7505FD0992CC5B638875D3BCA313CB2F5D7DE1F1D0B2CB3A3857E8"_hash256),
        RandomnessProviderTest::makeParams(
            DomainSeparationTag::TicketProductionDST,
            Buffer{1, 2},
            2,
            "EBB9FAA31980E474AD7D0D3D51369E76D7DB383945545F5A23075A27544D1412"_hash256),
        RandomnessProviderTest::makeParams(
            DomainSeparationTag::PoStDST,
            Buffer{1},
            3,
            "4FB8E678AF9CD7DD078EE47249A5AFC10D53DC3F548A22AAEE9DB4903A9D0212"_hash256)));

struct RandomnessProviderValuesTest : public RandomnessProviderBaseTest,
                                      public ::testing::Test {
  std::shared_ptr<CSPRNG> random_generator =
      std::make_shared<BoostRandomGenerator>();

  uint8_t getRandomByte() {
    auto &&bytes = random_generator->randomBytes(1);
    return bytes[0];
  }

  Randomness generateRandomnessValue() {
    auto tag = static_cast<DomainSeparationTag>(getRandomByte() % 3 + 1);
    Serialization s{random_generator->randomBytes(buffer_size)};
    ChainEpoch index(getRandomByte() % 100);

    return randomness_provider->deriveRandomness(tag, s, index);
  }

  int buffer_size = 100;
};

/**
 * @given randomness provider, new randomness value generator, iterations count
 * @when get a new randomness value, repeat `iterations count` times
 * @then all values are different
 */
TEST_F(RandomnessProviderValuesTest, DifferentValuesSuccess) {
  std::unordered_set<Randomness> values;

  constexpr int iterations_count = 100;
  for (auto i = 0; i < iterations_count; ++i) {
    auto &&randomness = generateRandomnessValue();
    if (values.find(randomness) != values.end()) {
      ASSERT_TRUE(false) << "map already has value " + randomness.toHex();
    } else {
      values.insert(randomness);
    }
  }
}

/**
 * @given RandomnessProvider instance as source of random values, max entropy
 * value for given source
 * @when get random bytes and calculate entropy
 * @then calculated entropy is not less than (max entropy - 2)
 */
TEST_F(RandomnessProviderValuesTest, CheckRandomnessEntropySuccess) {
  constexpr size_t BUFFER_SIZE = Hash256::size();

  auto &&buffer = generateRandomnessValue();

  auto max = fc::crypto::max_entropy(BUFFER_SIZE);
  auto ent = fc::crypto::entropy(buffer);

  ASSERT_GE(ent, max - 2) << "bad quality randomness source";
}
