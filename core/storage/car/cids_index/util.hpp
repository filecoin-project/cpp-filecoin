/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/operations.hpp>

#include "codec/uvarint.hpp"
#include "common/error_text.hpp"
#include "common/file.hpp"
#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "storage/car/car.hpp"
#include "storage/car/cids_index/cids_index.hpp"
#include "storage/car/cids_index/progress.hpp"
#include "storage/ipld/cids_ipld.hpp"

namespace fc::storage::cids_index {
  using ipld::CidsIpld;

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  inline Outcome<std::shared_ptr<CidsIpld>> loadOrCreateWithProgress(
      const std::string &car_path,
      bool writable,
      boost::optional<size_t> max_memory,
      IpldPtr ipld,
      common::Logger log) {
    if (!log) {
      log = spdlog::default_logger();
    }
    if (writable && !boost::filesystem::exists(car_path)) {
      Bytes header;
      car::writeHeader(header, {});
      OUTCOME_TRY(common::writeFile(car_path, header));
    }
    std::ifstream car_file{car_path, std::ios::ate | std::ios::binary};
    if (!car_file.good()) {
      log->error("open car failed: {}", car_path);
      return ERROR_TEXT("loadOrCreateWithProgress: open car failed");
    }
    uint64_t car_size = car_file.tellg();
    car_file.seekg(0);
    codec::uvarint::VarintDecoder header;
    if (!read(car_file, header)) {
      return ERROR_TEXT("loadOrCreateWithProgress: read header failed");
    }
    auto header_end{header.length + header.value};
    auto indexed_end{header_end};
    auto cids_path{car_path + ".cids"};
    std::shared_ptr<Index> index;
    if (boost::filesystem::exists(cids_path)) {
      if (auto _index{load(cids_path, max_memory)}) {
        index = _index.value();
      } else {
        log->error("index loading error: {:#}", _index.error());
      }
    }
    std::vector<MergeRange> ranges;
    std::ifstream index_file;
    if (index && (index->size() != 0)) {
      if (!readCarItem(car_file, index->info.max_offset, &indexed_end).first
          || indexed_end > car_size) {
        log->warn("index invalidated: {}", cids_path);
        index = nullptr;
        indexed_end = header_end;
      }
    }
    if (index && indexed_end < car_size) {
      car_file.seekg(gsl::narrow<int64_t>(indexed_end));
      codec::uvarint::VarintDecoder varint;
      if (!codec::uvarint::read(car_file, varint) || (varint.value == 0)
          || indexed_end + varint.length + varint.value > car_size) {
        car_size = indexed_end;
        boost::filesystem::resize_file(car_path, car_size);
      } else if (index->size() != 0) {
        auto &range{ranges.emplace_back()};
        range.begin = 1;
        range.end = 1 + index->size();
        index_file.open(cids_path, std::ios::binary);
        range.file = &index_file;
      }
    }
    if (!index || indexed_end < car_size) {
      Progress progress;
      if (Progress::isTty()) {
        // each 100,000 items
        progress.items.step = 100000;
        // each 64MB
        progress.car_offset.step = 64 << 20;
      } else {
        // each 1% or 1GB
        progress.car_offset.step = std::min<size_t>(car_size / 100, 1 << 30);
      }
      progress.car_size = car_size - indexed_end;
      auto _index{[&]() -> outcome::result<std::shared_ptr<Index>> {
        progress.begin();
        auto BOOST_OUTCOME_TRY_UNIQUE_NAME{
            gsl::finally([&] { progress.end(); })};
        auto rows_path{cids_path + ".tmp2"};
        std::fstream rows_file{
            rows_path,
            std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc};
        OUTCOME_TRY(readCar(car_file,
                            indexed_end,
                            car_size,
                            max_memory,
                            ipld,
                            &progress,
                            rows_file,
                            ranges));
        auto tmp_cids_path{cids_path + ".tmp"};
        if (ranges.size() == 1) {
          tmp_cids_path = rows_path;
        } else {
          progress.sort();
          std::ofstream index_file{tmp_cids_path, std::ios::binary};
          OUTCOME_TRY(merge(index_file, std::move(ranges)));
          boost::system::error_code ec;
          boost::filesystem::remove(rows_path, ec);
        }
        boost::system::error_code ec;
        boost::filesystem::rename(tmp_cids_path, cids_path, ec);
        if (ec) {
          return ec;
        }
        return load(cids_path, max_memory);
      }()};
      if (_index) {
        index = _index.value();
      } else {
        log->error("index generation error: {:#}", _index.error());
        return _index.error();
      }
      if (index->size() != 0) {
        if (!readCarItem(car_file, index->info.max_offset, &indexed_end).first
            || indexed_end > car_size) {
          return ERROR_TEXT("loadOrCreateWithProgress: invalid index");
        }
      }
      if (indexed_end < car_size) {
        car_file.seekg(gsl::narrow<int64_t>(indexed_end));
        codec::uvarint::VarintDecoder varint;
        if (!codec::uvarint::read(car_file, varint) || (varint.value == 0)
            || indexed_end + varint.length + varint.value > car_size) {
          car_size = indexed_end;
          boost::filesystem::resize_file(car_path, car_size);
        }
      }
    }
    auto _ipld{std::make_shared<CidsIpld>()};
    _ipld->car_file.open(car_path, std::ios::in | std::ios::binary);
    _ipld->index = index;
    _ipld->ipld = ipld;
    if (writable) {
      _ipld->writable = {fopen(car_path.c_str(), "ab"), fclose};
    }
    _ipld->car_offset = car_size;
    _ipld->car_path = car_path;
    _ipld->index_path = cids_path;
    _ipld->max_memory = max_memory;
    return _ipld;
  }
}  // namespace fc::storage::cids_index
