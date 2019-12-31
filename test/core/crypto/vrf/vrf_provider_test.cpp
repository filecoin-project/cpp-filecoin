/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/impl/vrf_provider_impl.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/address_utils.hpp"

using fc::common::Blob;
using fc::common::Buffer;
using fc::crypto::randomness::DomainSeparationTag;
using fc::crypto::vrf::VRFHash;
using fc::crypto::vrf::VRFHashProvider;
using fc::crypto::vrf::VRFProvider;
using fc::crypto::vrf::VRFProviderImpl;
using fc::primitives::address::Address;

struct VRFProviderTest : public ::testing::Test {
  void SetUp() override {
    vrf_provider = std::make_shared<VRFProviderImpl>();
  }
  std::shared_ptr<VRFProvider> vrf_provider;
};

/**
 * @given
 * @when
 * @then
 */
TEST_F(VRFProviderTest, VRFGenerateSuccess) {}

/**
 * @given
 * @when
 * @then
 */
TEST_F(VRFProviderTest, VRFGenerateFail) {}

/**
 * @given
 * @when
 * @then
 */
TEST_F(VRFProviderTest, VRFVerifySuccess) {}

/**
 * @given
 * @when
 * @then
 */
TEST_F(VRFProviderTest, VRFVerifyFail) {}
