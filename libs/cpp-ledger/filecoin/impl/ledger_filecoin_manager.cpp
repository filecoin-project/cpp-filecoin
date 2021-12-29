/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/filecoin/ledger_filecoin.hpp"

#include <iostream>
#include "cpp-ledger/filecoin/impl/ledger_filecoin_impl.hpp"
#include "cpp-ledger/ledger/ledger.hpp"

namespace ledger::filecoin {

  void LedgerFilecoinManager::ListFilecoinDevices(
      const std::vector<uint32_t> &path) const {
    const auto hid = CreateLedgerAdmin();

    for (int i = 0; i < hid->CountDevices(); i++) {
      const auto [device, hidErr] = hid->Connect(i);
      if (hidErr != std::nullopt) {
        continue;
      }

      LedgerFilecoinImpl app(device);
      const Defer defer([&app]() { app.Close(); });

      auto [version, err] = app.GetVersion();
      if (err != std::nullopt) {
        continue;
      }

      std::string address;
      std::tie(std::ignore, std::ignore, address, err) =
          app.GetAddressPubKeySECP256K1(path);

      if (err != std::nullopt) {
        continue;
      }

      std::cout << "============ Device found" << std::endl;
      std::cout << "Filecoin App Version : " << version.ToString() << std::endl;
      std::cout << "Filecoin App Address : " << address << std::endl;
    }
  }

  std::tuple<std::shared_ptr<LedgerFilecoin>, Error>
  LedgerFilecoinManager::ConnectLedgerFilecoinApp(
      const std::string &seekingAddress,
      const std::vector<uint32_t> &path) const {
    const auto hid = CreateLedgerAdmin();

    for (int i = 0; i < hid->CountDevices(); i++) {
      auto [device, err] = hid->Connect(i);
      if (err != std::nullopt) {
        continue;
      }

      const auto app = std::make_shared<LedgerFilecoinImpl>(device);

      std::string address;
      std::tie(std::ignore, std::ignore, address, err) =
          app->GetAddressPubKeySECP256K1(path);

      if (err != std::nullopt) {
        app->Close();
        continue;
      }

      if (seekingAddress.empty() || address == seekingAddress) {
        return std::make_tuple(app, Error{});
      } else {
        app->Close();
      }
    }

    return std::make_tuple(
        nullptr, Error{"no Filecoin app with specified address found"});
  }

  std::tuple<std::shared_ptr<LedgerFilecoin>, Error>
  LedgerFilecoinManager::FindLedgerFilecoinApp() const {
    const auto hid = CreateLedgerAdmin();
    auto [device, err] = hid->Connect(0);

    if (err != std::nullopt) {
      return std::make_tuple(nullptr, err);
    }

    const auto app = std::make_shared<LedgerFilecoinImpl>(device);

    auto [version, verErr] = app->GetVersion();

    if (verErr != std::nullopt) {
      if (verErr == "[APDU_CODE_CLA_NOT_SUPPORTED] Class not supported") {
        std::cout << "are you sure the Filecoin app is open?" << std::endl;
      }
      app->Close();
      return std::make_tuple(nullptr, err);
    }

    err = app->CheckVersion(version);
    if (err != std::nullopt) {
      app->Close();
      return std::make_tuple(nullptr, err);
    }

    return std::make_tuple(app, Error{});
  }

}  // namespace ledger::filecoin
