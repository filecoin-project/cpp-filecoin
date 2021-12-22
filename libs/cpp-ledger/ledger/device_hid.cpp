/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "device_hid.hpp"

namespace ledger {

  DeviceHid::DeviceHid() {
    info.path = nullptr;
    info.serial_number = nullptr;
    info.manufacturer_string = nullptr;
    info.product_string = nullptr;
    info.next = nullptr;
  }

  DeviceHid::~DeviceHid() {
    hid_close(device);

    if (info.path != nullptr) {
      delete[] info.path;
    }

    if (info.serial_number != nullptr) {
      delete[] info.serial_number;
    }

    if (info.manufacturer_string != nullptr) {
      delete[] info.manufacturer_string;
    }

    if (info.product_string != nullptr) {
      delete[] info.product_string;
    }
  }

  DeviceHid::DeviceHid(DeviceHid &&other) {
    if (this != &other) {
      info = std::move(other.info);
      device = std::move(other.device);
    }
  }

  DeviceHid &DeviceHid::operator=(DeviceHid &&other) {
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
    if (device == nullptr) {
      device = hid_open_path(info.path);
      if (device == nullptr) {
        return Error{"hidapi: failed to open device"};
      }
    }
    return Error{};
  }

  void DeviceHid::Close() {
    std::lock_guard lock(mutex);
    if (device != nullptr) {
      hid_close(device);
      device = nullptr;
    }
  }

  std::tuple<int, Error> DeviceHid::Write(const Bytes &bytes) const {
    if (bytes.empty()) {
      return std::make_tuple(0, Error{});
    }

    std::lock_guard lock(mutex);

    if (device == nullptr) {
      return std::make_tuple(0, Error{"hid : device closed"});
    }

    const auto written = hid_write(device, bytes.data(), bytes.size());
    if (written == -1) {
      const auto *message = hid_error(device);
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

    if (device == nullptr) {
      return std::make_tuple(0, Error{"hid : device closed"});
    }

    const auto read = hid_read(device, bytes.data(), bytes.size());
    if (read == -1) {
      const auto *message = hid_error(device);
      if (message == nullptr) {
        return std::make_tuple(0, Error{"hidapi: unknown failure"});
      }

      const std::string failure = convertToString(std::wstring(message));
      return std::make_tuple(0, "hidapi: " + failure);
    }

    return std::make_tuple(read, Error{});
  }

  std::vector<DeviceHid> Enumerate(unsigned short vendor_id,
                                   unsigned short product_id) {
    static std::mutex mutex;
    std::lock_guard lock(mutex);

    auto *head = hid_enumerate(vendor_id, product_id);
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
