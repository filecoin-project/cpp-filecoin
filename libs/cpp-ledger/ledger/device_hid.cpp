/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/ledger/device_hid.hpp"

#include <sstream>
#include "cpp-ledger/ledger/const.hpp"
#include "cpp-ledger/ledger/utils.hpp"

namespace ledger {

  DeviceHid::DeviceHid() {
    info.path = nullptr;
    info.serial_number = nullptr;
    info.manufacturer_string = nullptr;
    info.product_string = nullptr;
    info.next = nullptr;
  }

  DeviceHid::~DeviceHid() {
    delete[] info.path;
    delete[] info.serial_number;
    delete[] info.manufacturer_string;
    delete[] info.product_string;
  }

  DeviceHid::DeviceHid(DeviceHid &&other) noexcept {
    if (this != &other) {
      info = std::move(other.info);
      device = std::move(other.device);
    }
  }

  DeviceHid &DeviceHid::operator=(DeviceHid &&other) noexcept {
    if (this != &other) {
      info = std::move(other.info);
      device = std::move(other.device);
    }
    return *this;
  }

  void DeviceHid::SetInfo(const hid_device_info *device_info) {
    if (device_info != nullptr) {
      copyStr(info.path, device_info->path);
      info.vendor_id = device_info->vendor_id;
      info.product_id = device_info->product_id;
      copyStr(info.serial_number, device_info->serial_number);
      info.release_number = device_info->release_number;
      copyStr(info.manufacturer_string, device_info->manufacturer_string);
      copyStr(info.product_string, device_info->product_string);
      info.usage_page = device_info->usage_page;
      info.usage = device_info->usage;
      info.interface_number = device_info->interface_number;
    }
  }

  Error DeviceHid::Open() {
    std::lock_guard lock(mutex);
    if (!device) {
      auto *device_ptr = hid_open_path(info.path);
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
    const bool deviceFound = info.usage_page == kUsagePageLedgerNanoS;

    bool supported = false;
    if (kSupportedLedgerProductId.find(info.product_id)
        != kSupportedLedgerProductId.end()) {
      supported = kSupportedLedgerProductId.at(info.product_id)
                  == info.interface_number;
    }

    return deviceFound || supported;
  }

  std::string DeviceHid::ToString() const {
    std::stringstream ss;
    ss << "============ " << info.path << std::endl;
    ss << "VendorID      : " << std::hex << info.vendor_id << std::endl;
    ss << "ProductID     : " << std::hex << info.product_id << std::endl;
    ss << "Release       : " << std::hex << info.release_number << std::endl;

    ss << "Serial        : ";
    std::wstring serial(info.serial_number);
    for (wchar_t symbol : serial) {
      ss << std::hex << symbol;
    }
    ss << std::endl;

    ss << "Manufacturer  : " << convertToString(info.manufacturer_string)
       << std::endl;
    ss << "Product       : " << convertToString(info.product_string)
       << std::endl;
    ss << "UsagePage     : " << std::hex << info.usage_page << std::endl;
    ss << "Usage         : " << std::hex << info.usage << std::endl;
    return ss.str();
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
