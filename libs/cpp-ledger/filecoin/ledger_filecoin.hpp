/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <tuple>
#include "cpp-ledger/common/types.hpp"
#include "cpp-ledger/filecoin/types/signature_answer.hpp"
#include "cpp-ledger/filecoin/types/version_info.hpp"

namespace ledger::filecoin {

  // LedgerFilecoin represents a connection to the Ledger app
  class LedgerFilecoin {
   public:
    virtual ~LedgerFilecoin() = default;

    // Close closes a connection with the Filecoin user app
    virtual void Close() const = 0;

    // CheckVersion returns error if the App version is not supported by this
    // library and nothing if supported
    virtual Error CheckVersion(const VersionInfo &version) const = 0;

    // GetVersion returns the current version of the Filecoin user app
    virtual std::tuple<VersionInfo, Error> GetVersion() const = 0;

    // SignSECP256K1 signs a transaction using Filecoin user app
    // this command requires user confirmation in the device
    virtual std::tuple<SignatureAnswer, Error> SignSECP256K1(
        const std::vector<uint32_t> &bip44path,
        const Bytes &transaction) const = 0;

    // GetPublicKeySECP256K1 retrieves the public key for the corresponding
    // bip44 derivation path
    // this command does not require user confirmation in the device
    virtual std::tuple<Bytes, Error> GetPublicKeySECP256K1(
        const std::vector<uint32_t> &bip44path) const = 0;

    // GetAddressPubKeySECP256K1 returns the pubkey and addresses
    // this command does not require user confirmation
    virtual std::tuple<Bytes, Bytes, std::string, Error>
    GetAddressPubKeySECP256K1(const std::vector<uint32_t> &bip44path) const = 0;

    // ShowAddressPubKeySECP256K1 returns the pubkey (compressed) and addresses
    // this command requires user confirmation in the device
    virtual std::tuple<Bytes, Bytes, std::string, Error>
    ShowAddressPubKeySECP256K1(
        const std::vector<uint32_t> &bip44path) const = 0;
  };

  class LedgerFilecoinManager {
   public:
    // Displays existing Ledger Filecoin apps by address
    void ListFilecoinDevices(const std::vector<uint32_t> &path) const;

    // ConnectLedgerFilecoinApp connects to Filecoin app based on address
    std::tuple<std::shared_ptr<LedgerFilecoin>, Error> ConnectLedgerFilecoinApp(
        const std::string &seekingAddress,
        const std::vector<uint32_t> &path) const;

    // FindLedgerFilecoinApp finds the Filecoin app running in a Ledger device
    std::tuple<std::shared_ptr<LedgerFilecoin>, Error> FindLedgerFilecoinApp()
        const;
  };

}  // namespace ledger::filecoin
