/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cpp-ledger/filecoin/ledger_filecoin.hpp"

#include "cpp-ledger/ledger/ledger.hpp"

namespace ledger::filecoin {

  class LedgerFilecoinImpl : public LedgerFilecoin {
   public:
    explicit LedgerFilecoinImpl(std::shared_ptr<LedgerDevice> ledgerDevice);

    void Close() const override;

    Error CheckVersion(const VersionInfo &version) const override;

    std::tuple<VersionInfo, Error> GetVersion() const override;

    std::tuple<SignatureAnswer, Error> SignSECP256K1(
        const std::vector<uint32_t> &bip44path,
        const Bytes &transaction) const override;

    std::tuple<Bytes, Error> GetPublicKeySECP256K1(
        const std::vector<uint32_t> &bip44path) const override;

    std::tuple<Bytes, Bytes, std::string, Error> GetAddressPubKeySECP256K1(
        const std::vector<uint32_t> &bip44path) const override;

    std::tuple<Bytes, Bytes, std::string, Error> ShowAddressPubKeySECP256K1(
        const std::vector<uint32_t> &bip44path) const override;

   private:
    std::tuple<Bytes, Error> Sign(const std::vector<uint32_t> &bip44path,
                                  const Bytes &transaction) const;

    std::tuple<Bytes, Bytes, std::string, Error> RetrieveAddressPubKeySECP256K1(
        const std::vector<uint32_t> &bip44path, bool requireConfirmation) const;

    std::shared_ptr<LedgerDevice> device;
  };

}  // namespace ledger::filecoin
