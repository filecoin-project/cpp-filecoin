/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/filecoin/impl/ledger_filecoin_impl.hpp"

#include <gtest/gtest.h>

#include "cpp-ledger/filecoin/impl/utils.hpp"
#include "testutil/mocks/cpp-ledger/ledger_device_mock.hpp"

namespace ledger::filecoin {
  using testing::_;
  using testing::Invoke;
  using testing::Return;

  struct LedgerFilecoinTest : testing::Test {
    void SetUp() override {
      EXPECT_CALL(*device, Close()).WillRepeatedly(Return());
    }

    std::shared_ptr<MockLedgerDevice> device =
        std::make_shared<MockLedgerDevice>();
    LedgerFilecoinImpl app{device};
  };

  TEST_F(LedgerFilecoinTest, CheckVersion) {
    const VersionInfo version1{
        .appMode = 0, .major = 0, .minor = 2, .patch = 15};
    EXPECT_EQ(app.CheckVersion(version1),
              "App Version required 0.3.0 - Version found: 0.2.15");

    const VersionInfo version2{
        .appMode = 1, .major = 0, .minor = 2, .patch = 15};
    EXPECT_EQ(app.CheckVersion(version2),
              "App Version required 0.3.0 - Version found: 0.2.15");

    const VersionInfo version3{
        .appMode = 0, .major = 0, .minor = 3, .patch = 0};
    EXPECT_EQ(app.CheckVersion(version3), std::nullopt);

    const VersionInfo version4{
        .appMode = 0, .major = 1, .minor = 0, .patch = 0};
    EXPECT_EQ(app.CheckVersion(version4), std::nullopt);
  }

  TEST_F(LedgerFilecoinTest, GetVersion) {
    const Bytes message{kCLA, kINSGetVersion, 0, 0, 0};

    VersionInfo result;
    Error err;

    EXPECT_CALL(*device, Exchange(message))
        .WillOnce(Return(std::make_tuple(Bytes{}, Error{"test error"})));
    std::tie(result, err) = app.GetVersion();
    EXPECT_EQ(err, "test error");

    EXPECT_CALL(*device, Exchange(message))
        .WillOnce(Return(std::make_tuple(Bytes{1, 2}, Error{})));
    std::tie(result, err) = app.GetVersion();
    EXPECT_EQ(err, "invalid response");

    const VersionInfo expected_version{
        .appMode = 0, .major = 1, .minor = 2, .patch = 3};

    EXPECT_CALL(*device, Exchange(message))
        .WillOnce(Return(std::make_tuple(Bytes{0, 1, 2, 3}, Error{})));
    std::tie(result, err) = app.GetVersion();
    EXPECT_EQ(err, std::nullopt);
    EXPECT_EQ(result, expected_version);
  }

  TEST_F(LedgerFilecoinTest, WrongSign) {
    const std::vector<uint32_t> bip44path{44, 100, 0, 0, 0};
    const Bytes transaction{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    SignatureAnswer result;
    Error err;

    EXPECT_CALL(*device, Exchange(_))
        .WillRepeatedly(Return(std::make_tuple(Bytes{}, Error{"test error"})));
    std::tie(result, err) = app.SignSECP256K1(bip44path, transaction);
    EXPECT_EQ(err, "test error");

    EXPECT_CALL(*device, Exchange(_))
        .WillRepeatedly(Return(std::make_tuple(Bytes{1, 2, 3, 4, 5}, Error{})));
    std::tie(result, err) = app.SignSECP256K1(bip44path, transaction);
    EXPECT_EQ(err, "The signature provided is too short.");
  }

  TEST_F(LedgerFilecoinTest, Sign) {
    const std::vector<uint32_t> bip44path{44, 100, 0, 0, 0};
    Bytes transaction;
    Bytes response;

    SignatureAnswer expected_sign;

    for (int i = 0; i < 100; i++) {
      transaction.push_back(i);

      const Byte value = i + 50;
      response.push_back(value);

      if (i < 32) {
        expected_sign.r.push_back(value);
      }
      if (i >= 32 && i < 64) {
        expected_sign.s.push_back(value);
      }
      if (i == 64) {
        expected_sign.v = value;
      }
      if (i >= 65) {
        expected_sign.derSignature.push_back(value);
      }
    }

    EXPECT_CALL(*device, Exchange(_))
        .WillRepeatedly(Return(std::make_tuple(response, Error{})));
    const auto [result, err] = app.SignSECP256K1(bip44path, transaction);

    EXPECT_EQ(err, std::nullopt);
    EXPECT_EQ(result, expected_sign);
  }

  TEST_F(LedgerFilecoinTest, WrongAddressPubKey) {
    const std::vector<uint32_t> bip44path{44, 100, 0, 0, 0};
    const auto [pathBytes, skipErr] = getBip44bytes(bip44path, kHardenCount);

    Bytes pubkey;
    Bytes address;
    std::string addressStr;
    Error err;

    constexpr Byte testAddrLen = 21;
    constexpr Byte testAddrStrLen = 41;

    Bytes message{
        kCLA, kINSGetAddrSECP256K1, 0, 0, static_cast<Byte>(pathBytes.size())};
    message.insert(message.end(), pathBytes.begin(), pathBytes.end());

    EXPECT_CALL(*device, Exchange(message))
        .WillRepeatedly(Return(std::make_tuple(Bytes{}, Error{"temp error"})));
    std::tie(pubkey, address, addressStr, err) =
        app.GetAddressPubKeySECP256K1(bip44path);
    EXPECT_EQ(err, "temp error");

    Bytes response{0, 1, 2, 3};
    response.reserve(kPublicKeyLength + 1 + testAddrLen + 1 + testAddrStrLen);

    EXPECT_CALL(*device, Exchange(message)).WillRepeatedly(Invoke([&](auto &) {
      return std::make_tuple(response, Error{});
    }));
    std::tie(pubkey, address, addressStr, err) =
        app.GetAddressPubKeySECP256K1(bip44path);
    EXPECT_EQ(err, "Invalid response");  // too short pubkey

    response.resize(kPublicKeyLength, 1);
    response.push_back(testAddrLen);  // address length
    response.push_back(3);
    std::tie(pubkey, address, addressStr, err) =
        app.GetAddressPubKeySECP256K1(bip44path);
    EXPECT_EQ(err, "Invalid response");  // too short address

    response.resize(kPublicKeyLength + 1 + testAddrLen, 1);
    response.push_back(testAddrStrLen);  // address string length
    response.push_back(5);
    std::tie(pubkey, address, addressStr, err) =
        app.GetAddressPubKeySECP256K1(bip44path);
    EXPECT_EQ(err, "Invalid response");  // too short address string
  }

  TEST_F(LedgerFilecoinTest, GetAddressPubKey) {
    const std::vector<uint32_t> bip44path{44, 100, 0, 0, 0};
    const auto [pathBytes, skipErr] = getBip44bytes(bip44path, kHardenCount);

    Bytes message{
        kCLA, kINSGetAddrSECP256K1, 0, 0, static_cast<Byte>(pathBytes.size())};
    message.insert(message.end(), pathBytes.begin(), pathBytes.end());

    const Bytes expected_pubkey{
        0x04, 0xe6, 0xa2, 0x62, 0xc9, 0x6c, 0x7d, 0x7f, 0xd0, 0x15, 0x27,
        0x3e, 0xc4, 0x69, 0x49, 0x2c, 0x26, 0x26, 0xeb, 0x2e, 0x29, 0xd7,
        0x3e, 0x7f, 0x65, 0xc6, 0x4d, 0x69, 0x56, 0x70, 0x34, 0x3a, 0xaa,
        0x64, 0xec, 0x95, 0x51, 0xc7, 0x3a, 0xdf, 0x8c, 0xa2, 0x16, 0xb3,
        0x6c, 0x17, 0x20, 0xd9, 0xd7, 0x00, 0xda, 0x99, 0x1c, 0x89, 0x9c,
        0x12, 0x9c, 0x37, 0x15, 0x40, 0x6f, 0x06, 0x0f, 0x1b, 0xd4};

    const Bytes expected_address{0x01, 0x44, 0x60, 0x3d, 0x82, 0x38, 0x28,
                                 0x85, 0x56, 0x7f, 0x72, 0x9c, 0x11, 0xf2,
                                 0x6d, 0xe7, 0x5b, 0xe6, 0x05, 0x22, 0xb1};

    const std::string expected_addressStr{
        "f1irqd3aryfccvm73stqi7e3phlptakivru5mirnq"};

    Bytes response{expected_pubkey};
    response.reserve(expected_pubkey.size() + 1 + expected_address.size() + 1
                     + expected_addressStr.size());

    response.push_back(expected_address.size());
    response.insert(
        response.end(), expected_address.begin(), expected_address.end());

    response.push_back(expected_addressStr.size());
    response.insert(
        response.end(), expected_addressStr.begin(), expected_addressStr.end());

    EXPECT_CALL(*device, Exchange(message))
        .WillRepeatedly(Return(std::make_tuple(response, Error{})));

    const auto [pubkey, address, addressStr, err] =
        app.GetAddressPubKeySECP256K1(bip44path);

    EXPECT_EQ(err, std::nullopt);
    EXPECT_EQ(pubkey, expected_pubkey);
    EXPECT_EQ(address, expected_address);
    EXPECT_EQ(addressStr, expected_addressStr);
  }

  TEST_F(LedgerFilecoinTest, ShowAddressPubKey) {
    const std::vector<uint32_t> bip44path{44, 100, 0, 0, 0};
    const auto [pathBytes, skipErr] = getBip44bytes(bip44path, kHardenCount);

    Bytes message{
        kCLA, kINSGetAddrSECP256K1, 1, 0, static_cast<Byte>(pathBytes.size())};
    message.insert(message.end(), pathBytes.begin(), pathBytes.end());

    const Bytes expected_pubkey{
        0x04, 0xe6, 0xa2, 0x62, 0xc9, 0x6c, 0x7d, 0x7f, 0xd0, 0x15, 0x27,
        0x3e, 0xc4, 0x69, 0x49, 0x2c, 0x26, 0x26, 0xeb, 0x2e, 0x29, 0xd7,
        0x3e, 0x7f, 0x65, 0xc6, 0x4d, 0x69, 0x56, 0x70, 0x34, 0x3a, 0xaa,
        0x64, 0xec, 0x95, 0x51, 0xc7, 0x3a, 0xdf, 0x8c, 0xa2, 0x16, 0xb3,
        0x6c, 0x17, 0x20, 0xd9, 0xd7, 0x00, 0xda, 0x99, 0x1c, 0x89, 0x9c,
        0x12, 0x9c, 0x37, 0x15, 0x40, 0x6f, 0x06, 0x0f, 0x1b, 0xd4};

    const Bytes expected_address{0x01, 0x44, 0x60, 0x3d, 0x82, 0x38, 0x28,
                                 0x85, 0x56, 0x7f, 0x72, 0x9c, 0x11, 0xf2,
                                 0x6d, 0xe7, 0x5b, 0xe6, 0x05, 0x22, 0xb1};

    const std::string expected_addressStr{
        "f1irqd3aryfccvm73stqi7e3phlptakivru5mirnq"};

    Bytes response{expected_pubkey};
    response.reserve(expected_pubkey.size() + 1 + expected_address.size() + 1
                     + expected_addressStr.size());

    response.push_back(expected_address.size());
    response.insert(
        response.end(), expected_address.begin(), expected_address.end());

    response.push_back(expected_addressStr.size());
    response.insert(
        response.end(), expected_addressStr.begin(), expected_addressStr.end());

    EXPECT_CALL(*device, Exchange(message))
        .WillRepeatedly(Return(std::make_tuple(response, Error{})));

    const auto [pubkey, address, addressStr, err] =
        app.ShowAddressPubKeySECP256K1(bip44path);

    EXPECT_EQ(err, std::nullopt);
    EXPECT_EQ(pubkey, expected_pubkey);
    EXPECT_EQ(address, expected_address);
    EXPECT_EQ(addressStr, expected_addressStr);
  }

}  // namespace ledger::filecoin
