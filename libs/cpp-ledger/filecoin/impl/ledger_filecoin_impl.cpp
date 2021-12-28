/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/filecoin/impl/ledger_filecoin_impl.hpp"

#include "cpp-ledger/filecoin/impl/utils.hpp"

namespace ledger::filecoin {

  const VersionInfo requiredVersion{
      .appMode = 0, .major = 0, .minor = 3, .patch = 0};

  constexpr Byte kCLA = 0x06;

  constexpr Byte kINSGetVersion = 0;
  constexpr Byte kINSGetAddrSECP256K1 = 1;
  constexpr Byte kINSSignSECP256K1 = 2;

  constexpr Byte kPayloadChunkInit = 0;
  constexpr Byte kPayloadChunkAdd = 1;
  constexpr Byte kPayloadChunkLast = 2;

  constexpr int kHardenCount = 2;

  constexpr size_t kMinResponseLength = 4;
  constexpr size_t kMinSignLength = 66;

  constexpr size_t kPublicKeyLength = 65;

  LedgerFilecoinImpl::LedgerFilecoinImpl(
      const std::shared_ptr<LedgerDevice> &ledgerDevice)
      : device(ledgerDevice) {}

  void LedgerFilecoinImpl::Close() const {
    device->Close();
  }

  std::tuple<bool, Error> LedgerFilecoinImpl::CheckVersion(
      const VersionInfo &version) const {
    return (version < requiredVersion) ? std::make_tuple(
               false,
               Error{"App Version required " + requiredVersion.ToString()
                     + " - Version found: " + version.ToString()})
                                       : std::make_tuple(true, Error{});
  }

  std::tuple<VersionInfo, Error> LedgerFilecoinImpl::GetVersion() const {
    const Bytes message{kCLA, kINSGetVersion, 0, 0, 0};
    const auto [response, err] = device->Exchange(message);

    if (err != std::nullopt) {
      return std::make_tuple(VersionInfo{}, err);
    }

    if (response.size() < kMinResponseLength) {
      return std::make_tuple(VersionInfo{}, Error{"invalid response"});
    }

    const VersionInfo version{.appMode = response[0],
                              .major = response[1],
                              .minor = response[2],
                              .patch = response[3]};

    return std::make_tuple(version, Error{});
  }

  std::tuple<SignatureAnswer, Error> LedgerFilecoinImpl::SignSECP256K1(
      const std::vector<uint32_t> &bip44path, const Bytes &transaction) const {
    const auto [signBytes, err] = Sign(bip44path, transaction);

    if (err != std::nullopt) {
      return std::make_tuple(SignatureAnswer{}, err);
    }

    if (signBytes.size() < kMinSignLength) {
      return std::make_tuple(SignatureAnswer{},
                             Error{"The signature provided is too short."});
    }

    const SignatureAnswer sign{
        .r = {signBytes.begin(), signBytes.begin() + 32},
        .s = {signBytes.begin() + 32, signBytes.begin() + 64},
        .v = signBytes.at(64),
        .derSignature = {signBytes.begin() + 65, signBytes.end()}};

    return std::make_tuple(sign, Error{});
  }

  std::tuple<Bytes, Error> LedgerFilecoinImpl::GetPublicKeySECP256K1(
      const std::vector<uint32_t> &bip44path) const {
    Bytes pubkey;
    Error err;

    std::tie(pubkey, std::ignore, std::ignore, err) =
        RetrieveAddressPubKeySECP256K1(bip44path, false);

    return std::make_tuple(pubkey, err);
  }

  std::tuple<Bytes, Bytes, std::string, Error>
  LedgerFilecoinImpl::GetAddressPubKeySECP256K1(
      const std::vector<uint32_t> &bip44path) const {
    return RetrieveAddressPubKeySECP256K1(bip44path, false);
  }

  std::tuple<Bytes, Bytes, std::string, Error>
  LedgerFilecoinImpl::ShowAddressPubKeySECP256K1(
      const std::vector<uint32_t> &bip44path) const {
    return RetrieveAddressPubKeySECP256K1(bip44path, true);
  }

  std::tuple<Bytes, Error> LedgerFilecoinImpl::Sign(
      const std::vector<uint32_t> &bip44path, const Bytes &transaction) const {
    // todo
    return std::make_tuple(Bytes{}, Error{});
  }

  std::tuple<Bytes, Bytes, std::string, Error>
  LedgerFilecoinImpl::RetrieveAddressPubKeySECP256K1(
      const std::vector<uint32_t> &bip44path, bool requireConfirmation) const {
    const auto [pathBytes, err] = getBip44bytes(bip44path, kHardenCount);

    if (err != std::nullopt) {
      return std::make_tuple(Bytes{}, Bytes{}, "", err);
    }

    const Byte confirm = requireConfirmation ? 1 : 0;

    Bytes message{kCLA,
                  kINSGetAddrSECP256K1,
                  confirm,
                  0,
                  static_cast<Byte>(pathBytes.size())};

    message.insert(message.end(), pathBytes.begin(), pathBytes.end());

    const auto [response, exErr] = device->Exchange(message);

    if (exErr != std::nullopt) {
      return std::make_tuple(Bytes{}, Bytes{}, "", exErr);
    }

    const Error invalidResponse{"Invalid response"};

    size_t totalSize = kPublicKeyLength + 1;
    if (response.size() < totalSize) {
      return std::make_tuple(Bytes{}, Bytes{}, "", invalidResponse);
    }

    size_t cursor = 0;

    // Read pubkey
    const Bytes pubkey(response.begin(), response.begin() + kPublicKeyLength);
    cursor += kPublicKeyLength;

    // Read addr byte format length
    const size_t addrByteLength = response.at(cursor);
    cursor++;

    totalSize += addrByteLength + 1;
    if (response.size() < totalSize) {
      return std::make_tuple(Bytes{}, Bytes{}, "", invalidResponse);
    }

    // Read address bytes format
    const Bytes address(response.begin() + cursor,
                        response.begin() + cursor + addrByteLength);
    cursor += addrByteLength;

    // Read addr strin format length
    const size_t addrStringLength = response.at(cursor);
    cursor++;

    totalSize += addrStringLength;
    if (response.size() < totalSize) {
      return std::make_tuple(Bytes{}, Bytes{}, "", invalidResponse);
    }

    // Read addr string format
    const std::string addressStr(response.begin() + cursor,
                                 response.begin() + cursor + addrByteLength);

    return std::make_tuple(pubkey, address, addressStr, Error{});
  }

}  // namespace ledger::filecoin
