/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/operations.hpp>

#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "storage/car/cids_index/cids_index.hpp"
#include "storage/car/cids_index/progress.hpp"

namespace fc::storage::cids_index {
  inline outcome::result<std::shared_ptr<CidsIpld>> loadOrCreateWithProgress(
      const std::string &car_path,
      size_t max_memory,
      IpldPtr ipld,
      common::Logger log) {
    if (!log) {
      log = spdlog::default_logger();
    }
    auto cids_path{car_path + ".cids"};
    std::shared_ptr<Index> index;
    if (boost::filesystem::exists(cids_path)) {
      log->info("loading index");
      if (auto _index{load(cids_path, max_memory)}) {
        index = _index.value();
        log->info("index loaded: {}", cids_path);
      } else {
        log->error("index loading error: {:#}", _index.error());
      }
    }
    if (!index) {
      log->info("generating index");
      Progress progress;
      if (Progress::isTty()) {
        // each 100,000 items
        progress.items.step = 100000;
        // each 64MB
        progress.car_offset.step = 64 << 20;
      } else {
        // each 1% or 1GB
        progress.car_offset.step = std::min<size_t>(
            boost::filesystem::file_size(car_path) / 100, 1 << 30);
      }
      if (auto _index{create(car_path, cids_path, nullptr, &progress)}) {
        index = _index.value();
        log->info("index generated: {}", cids_path);
      } else {
        log->error("index generation error: {:#}", _index.error());
        return _index.error();
      }
    }
    return std::make_shared<CidsIpld>(car_path, index, ipld);
  }
}  // namespace fc::storage::cids_index
