/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/ledger/device_hid.hpp"

#include "cpp-ledger/ledger/const.hpp"
#include "cpp-ledger/ledger/utils.hpp"

namespace ledger {

  DeviceHid::DeviceHid(DeviceHid &&other) noexcept {
    if (this != &other) {
      info = other.info;
      device = std::move(other.device);
    }
  }

  DeviceHid &DeviceHid::operator=(DeviceHid &&other) noexcept {
    if (this != &other) {
      info = other.info;
      device = std::move(other.device);
    }
    return *this;
  }

  void DeviceHid::SetInfo(const hid_device_info *deviceInfo) {
    info = DeviceHidInfo(deviceInfo);
  }

  Error DeviceHid::Open() {
    std::lock_guard lock(mutex);
    if (!device) {
      auto *device_ptr = hid_open_path(info.path.c_str());
      if (device_ptr == nullptr) {
        return Error{"hidapi: failed to open device"};
      }
      device = std::unique_ptr<hid_device, DeviceDeleter>(
          device_ptr, [](hid_device *ptr) { hid_close(ptr); });
    }
    return Error{};
  }

  void DeviceHid::Close() {
    std::lock_guard lock(mutex);
    if (device) {
      auto *device_ptr = device.release();
      hid_close(device_ptr);
    }
  }

  std::tuple<int, Error> DeviceHid::Write(const Bytes &bytes) const {
    if (bytes.empty()) {
      return std::make_tuple(0, Error{});
    }

    std::lock_guard lock(mutex);

    if (!device) {
      return std::make_tuple(0, Error{"hid : device closed"});
    }

    const auto written = hid_write(device.get(), bytes.data(), bytes.size());
    if (written == -1) {
      const auto *message = hid_error(device.get());
      if (message == nullptr) {
        return std::make_tuple(0, Error{"hidapi: unknown failure"});
      }

      const std::string failure = convertToString(std::wstring(message));
      return std::make_tuple(0, "hidapi: " + failure);
    }
    return std::make_tuple(written, Error{});
  }

  std::tuple<int, Error> DeviceHid::Read(Bytes &bytes) const {
    if (bytes.empty()) {
      // The buffer must be allocated for reading
      return std::make_tuple(0, Error{});
    }

    std::lock_guard lock(mutex);

    if (!device) {
      return std::make_tuple(0, Error{"hid : device closed"});
    }

    const auto read = hid_read(device.get(), bytes.data(), bytes.size());
    if (read == -1) {
      const auto *message = hid_error(device.get());
      if (message == nullptr) {
        return std::make_tuple(0, Error{"hidapi: unknown failure"});
      }

      const std::string failure = convertToString(std::wstring(message));
      return std::make_tuple(0, Error{"hidapi: " + failure});
    }

    return std::make_tuple(read, Error{});
  }

  bool DeviceHid::IsLedgerDevice() const {
    const bool deviceFound = info.usagePage == kUsagePageLedgerNanoS;

    bool supported = false;
    if (kSupportedLedgerProductId.find(info.productId)
        != kSupportedLedgerProductId.end()) {
      supported =
          kSupportedLedgerProductId.at(info.productId) == info.interfaceNumber;
    }

    return deviceFound || supported;
  }

  std::string DeviceHid::ToString() const {
    return info.ToString();
  }

  std::vector<DeviceHid> Enumerate(uint16_t vendorId, uint16_t productId) {
    static std::mutex mutex;
    std::lock_guard lock(mutex);

    auto *head = hid_enumerate(vendorId, productId);
    if (head == nullptr) {
      return {};
    }

    std::vector<DeviceHid> devices;
    for (auto *info = head; info != nullptr; info = info->next) {
      DeviceHid device;
      device.SetInfo(info);
      devices.push_back(std::move(device));
    }

    hid_free_enumeration(head);
    return devices;
  }

}  // namespace ledger
