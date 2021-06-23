/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/file.hpp"

#include <boost/endian/buffers.hpp>
#include <boost/filesystem/operations.hpp>
#include <random>

#include "codec/cbor/light_reader/block.hpp"
#include "common/file.hpp"

#define BOOL_TRY(...)                 \
  do {                                \
    if (!(__VA_ARGS__)) return false; \
  } while (0)

namespace fc::primitives::tipset::chain::file {
  using boost::endian::big_uint64_buf_t;
  using Seed = BytesN<32>;

  Updater::operator bool() const {
    return hash && count;
  }
  Updater &Updater::apply(gsl::span<const CbCid> ts) {
    assert(ts.size() < kRevert);
    common::write(hash, ts);
    count.put(ts.size());
    return *this;
  }
  Updater &Updater::revert() {
    count.put(kRevert);
    return *this;
  }
  Updater &Updater::flush() {
    hash.flush();
    count.flush();
    return *this;
  }

  bool write(const std::string &path_hash,
             const std::string &path_count,
             CbCidsIn hashes,
             uint64_t min_height,
             BytesIn counts) {
    auto path_hash_tmp{path_hash + ".tmp"};
    auto path_count_tmp{path_count + ".tmp"};
    Seed seed;
    std::generate(
        seed.begin(),
        seed.end(),
        std::independent_bits_engine<std::random_device, 8, uint8_t>{});

    std::ofstream file_hash{path_hash_tmp};
    BOOL_TRY(common::write1(file_hash, seed));
    BOOL_TRY(common::write(file_hash, hashes));
    BOOL_TRY(file_hash.flush());
    file_hash.close();

    std::ofstream file_count{path_count_tmp};
    BOOL_TRY(common::write1(file_count, seed));
    BOOL_TRY(common::write1(file_count, big_uint64_buf_t{min_height}));
    BOOL_TRY(common::write(file_count, counts));
    BOOL_TRY(file_count.flush());
    file_count.close();

    boost::filesystem::rename(path_hash_tmp, path_hash);
    boost::filesystem::rename(path_count_tmp, path_count);
    return true;
  }

  bool load(TsChain &chain,
            const std::string &path_hash,
            const std::string &path_count) {
    chain = {};
    std::ifstream file_hash{path_hash, std::ios::ate};
    BOOL_TRY(file_hash);
    auto size_hash{(size_t)file_hash.tellg()};
    file_hash.seekg(0);
    Seed seed_hash;
    constexpr size_t header_hash_size{sizeof(seed_hash)};
    BOOL_TRY(size_hash > header_hash_size);
    size_hash -= header_hash_size;

    std::ifstream file_count{path_count, std::ios::ate};
    BOOL_TRY(file_count);
    auto size_count{(size_t)file_count.tellg()};
    file_count.seekg(0);
    Seed seed_count;
    big_uint64_buf_t endian;
    constexpr size_t header_count_size{sizeof(seed_count) + sizeof(endian)};
    BOOL_TRY(size_count > header_count_size);
    size_count -= header_count_size;

    BOOL_TRY(common::read1(file_count, seed_count));
    BOOL_TRY(common::read1(file_count, endian));
    const auto min_height{endian.value()};
    Bytes counts;
    counts.resize(size_count);
    BOOL_TRY(common::read(file_count, counts));
    std::vector<CbCid> hashes;
    hashes.resize(size_hash / sizeof(CbCid));
    BOOL_TRY(common::read1(file_hash, seed_hash));
    BOOL_TRY(seed_count == seed_hash);
    BOOL_TRY(common::read(file_hash, gsl::make_span(hashes)));

    auto hash_out{hashes.begin()}, hash_in{hash_out};
    auto count_out{counts.begin()};
    for (auto count_in{count_out}; count_in != counts.end(); ++count_in) {
      const auto count{*count_in};
      if (count == kRevert) {
        BOOL_TRY(count_out != counts.begin());
        --count_out;
        hash_out -= *count_out;
        *count_out = 0;
        while (count_out != counts.begin() && !*std::prev(count_out)) {
          --count_out;
        }
        BOOL_TRY(count_out != counts.begin() && *std::prev(count_out));
      } else {
        if (count_out != count_in) {
          *count_out = *count_in;
        }
        auto i{count_out - counts.begin()};
        BOOL_TRY(i || count);
        if (count > hashes.end() - hash_in) {
          BOOL_TRY(i);
          boost::filesystem::resize_file(path_count, header_count_size + i);
          break;
        }
        if (count) {
          if (hash_out != hash_in) {
            hash_out = std::copy_n(hash_in, count, hash_out);
          } else {
            hash_out += count;
          }
          hash_in += count;
        }
        ++count_out;
      }
    }
    if (count_out != counts.end()) {
      counts.erase(count_out, counts.end());
      hashes.erase(hash_out, hashes.end());
      BOOL_TRY(write(path_hash, path_count, hashes, min_height, counts));
    } else if (hashes.size() * sizeof(CbCid) < size_hash) {
      // maybe recover after interrupted revert-apply
      boost::filesystem::resize_file(
          path_hash, header_hash_size + hashes.size() * sizeof(CbCid));
    }
    auto height{min_height};
    hash_in = hashes.begin();
    // TODO(turuslan): lazy
    for (auto &count : counts) {
      if (count) {
        chain.emplace(height, TsLazy{{{hash_in, hash_in + count}}, {}});
        hash_in += count;
      }
      ++height;
    }
    return true;
  }

  struct Walk {
    CbIpldPtr ipld;
    std::vector<uint8_t> counts;
    BlockParentCbCids parents, _parents;
    uint64_t min_height{}, max_height{};
    Hash256 max_ticket;
    Buffer _block;
    bool first_ts{true};

    bool step(CbCidsIn tsk) {
      BOOL_TRY(tsk.size() < kRevert);
      auto first_block{true};
      for (auto &cid : tsk) {
        BOOL_TRY(ipld->get(cid, _block));
        BytesIn block{_block};
        BytesIn ticket;
        uint64_t height{};
        BOOL_TRY(codec::cbor::light_reader::readBlock(
            ticket, _parents, height, block));
        auto _ticket{crypto::blake2b::blake2b_256(ticket)};
        if (first_block) {
          if (first_ts) {
            max_height = height;
            counts.reserve(max_height + 1);
          } else {
            BOOL_TRY(height < min_height);
          }
          min_height = height;
          parents = _parents;
        } else {
          BOOL_TRY(height == min_height);
          BOOL_TRY(_parents == parents);
          BOOL_TRY(_ticket > max_ticket);
        }
        first_block = false;
      }
      counts.resize(max_height - min_height);
      counts.push_back(tsk.size());
      first_ts = false;
      return true;
    }
  };

  // TODO(turuslan): lazy
  bool loadOrCreate(TsChain &chain,
                    Updater *updater,
                    bool *updated,
                    const std::string &path,
                    const CbIpldPtr &ipld,
                    const std::vector<CbCid> &head_tsk,
                    size_t update_when) {
    if (updated) {
      *updated = false;
    }
    auto path_hash{path + ".hash"};
    auto path_count{path + ".count"};
    if (!load(chain, path_hash, path_count)) {
      std::vector<CbCid> hashes;
      BOOL_TRY(!head_tsk.empty());
      auto tsk{head_tsk};
      Walk walk;
      walk.ipld = ipld;
      do {
        BOOL_TRY(walk.step(tsk));
        // TODO(turuslan): lazy
        chain.emplace(walk.min_height, TsLazy{tsk, {}});
        hashes.insert(hashes.end(), tsk.rbegin(), tsk.rend());
        tsk = walk.parents;
      } while (walk.min_height);
      std::reverse(walk.counts.begin(), walk.counts.end());
      std::reverse(hashes.begin(), hashes.end());
      BOOL_TRY(
          write(path_hash, path_count, hashes, walk.min_height, walk.counts));
      if (updated) {
        *updated = true;
      }
    }
    if (updater) {
      *updater = {};
      updater->hash.open(path_hash, std::ios::app);
      updater->count.open(path_count, std::ios::app);
      BOOL_TRY(*updater);
      if (update_when) {
        // TODO(turuslan): lazy
        BOOL_TRY(!head_tsk.empty());
        auto tsk{head_tsk};
        Walk walk;
        walk.ipld = ipld;
        BOOL_TRY(walk.step(tsk));
        if (walk.min_height >= chain.rbegin()->first + update_when) {
          auto it{std::prev(chain.end())};
          while (walk.min_height != it->first || tsk != it->second.key.cids()) {
            if (it->first < walk.min_height) {
              chain.emplace(walk.min_height, TsLazy{tsk, {}});
              tsk = walk.parents;
              BOOL_TRY(walk.step(tsk));
            } else {
              chain.erase(it--);
              BOOL_TRY(updater->revert());
              if (updated) {
                *updated = true;
              }
            }
          }
          auto height{it->first};
          for (++it; it != chain.end(); ++it) {
            while (++height < it->first) {
              BOOL_TRY(updater->apply({}));
            }
            BOOL_TRY(updater->apply(it->second.key.cids()));
            if (updated) {
              *updated = true;
            }
          }
          BOOL_TRY(updater->flush());
        }
      }
    }
    return true;
  }
  TsBranchPtr loadOrCreate(bool *updated,
                           const std::string &path,
                           const CbIpldPtr &ipld,
                           const std::vector<CbCid> &head_tsk,
                           size_t update_when) {
    auto branch{std::make_shared<TsBranch>()};
    branch->updater = std::make_shared<Updater>();
    if (loadOrCreate(branch->chain,
                     branch->updater.get(),
                     updated,
                     path,
                     ipld,
                     head_tsk,
                     update_when)) {
      return branch;
    }
    return nullptr;
  }
}  // namespace fc::primitives::tipset::chain::file
