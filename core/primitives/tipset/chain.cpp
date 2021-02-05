/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/chain.hpp"

#include <boost/endian/conversion.hpp>

#include "common/outcome2.hpp"

namespace fc::primitives::tipset::chain {
  using common::Hash256;

  auto decodeHeight(BytesIn key) {
    assert(key.size() == sizeof(Height));
    return boost::endian::load_big_u64(key.data());
  }

  TipsetKey decodeTsk(BytesIn input) {
    std::vector<CID> cids;
    while (!input.empty()) {
      cids.push_back(asCborBlakeCid(
          Hash256::fromSpan(input.subspan(0, Hash256::size())).value()));
      input = input.subspan(Hash256::size());
    }
    return cids;
  }

  auto encodeHeight(Height height) {
    return Buffer{}.putUint64(height);
  }

  auto encodeTsk(const TipsetKey &tsk) {
    Buffer out;
    for (auto &cid : tsk.cids()) {
      out.put(*asBlake(cid));
    }
    return out;
  }

  TsBranchPtr TsBranch::make(TsChain chain, TsBranchPtr parent) {
    auto branch{std::make_shared<TsBranch>()};
    if (parent) {
      auto bottom{chain.begin()};
      assert(*parent->chain.find(bottom->first) == *bottom);
      if (chain.size() == 1) {
        return parent;
      }
      ++bottom;
      auto _parent{parent->chain.find(bottom->first)};
      assert(_parent == parent->chain.end() || *_parent != *bottom);
      parent->children.push_back(branch);
    }
    branch->chain = std::move(chain);
    branch->parent = std::move(parent);
    return branch;
  }

  outcome::result<TsBranchPtr> TsBranch::make(TsLoadPtr ts_load,
                                              const TipsetKey &key,
                                              TsBranchPtr parent) {
    TsChain chain;
    OUTCOME_TRY(ts, ts_load->load(key));
    auto _parent{parent->chain.lower_bound(ts->height())};
    if (_parent == parent->chain.end()) {
      --_parent;
    }
    while (true) {
      auto _bottom{chain.emplace(ts->height(), TsLazy{ts->key, ts}).first};
      while (_parent->first > _bottom->first) {
        if (_parent == parent->chain.begin()) {
          return OutcomeError::kDefault;
        }
        --_parent;
      }
      if (*_parent == *_bottom) {
        break;
      }
      OUTCOME_TRYA(ts, ts_load->load(ts->getParents()));
    }
    return make(std::move(chain), parent);
  }

  outcome::result<TsBranchPtr> TsBranch::load(KvPtr kv) {
    TsChain chain;
    auto cur{kv->cursor()};
    for (cur->seekToFirst(); cur->isValid(); cur->next()) {
      chain.emplace(decodeHeight(cur->key()), TsLazy{decodeTsk(cur->value())});
    }
    if (chain.empty()) {
      return nullptr;
    }
    return make(std::move(chain), nullptr);
  }

  outcome::result<TsBranchPtr> TsBranch::create(KvPtr kv,
                                                const TipsetKey &key,
                                                TsLoadPtr ts_load) {
    TsChain chain;
    auto batch{kv->batch()};
    OUTCOME_TRY(ts, ts_load->load(key));
    while (true) {
      OUTCOME_TRY(batch->put(encodeHeight(ts->height()), encodeTsk(ts->key)));
      chain.emplace(ts->height(), TsLazy{ts->key, ts});
      if (ts->height() == 0) {
        break;
      }
      OUTCOME_TRYA(ts, ts_load->load(ts->getParents()));
    }
    OUTCOME_TRY(batch->commit());
    return make(std::move(chain), nullptr);
  }

  outcome::result<Path> findPath(TsBranchPtr from, TsBranchIter to_it) {
    Path path;
    auto &[revert, apply]{path};
    auto &[to, _to]{to_it};
    apply.insert(*_to);
    while (to != from) {
      if (!to->parent) {
        return OutcomeError::kDefault;
      }
      auto bottom{to->chain.begin()};
      apply.insert(bottom, _to);
      _to = to->parent->chain.find(bottom->first);
      to = to->parent;
    }
    revert.insert(_to, from->chain.end());
    return path;
  }

  outcome::result<void> update(TsBranchPtr branch, const Path &path, KvPtr kv) {
    auto &from{branch->chain};
    auto &[revert, apply]{path};
    auto revert_to{from.find(revert.begin()->first)};

    assert(*revert.begin() == *revert_to);
    assert(*apply.begin() == *revert_to);
    assert(*revert.rbegin() == *from.rbegin());

    if (kv) {
      auto batch{kv->batch()};
      for (auto it{revert.rbegin()}; it != revert.rend(); ++it) {
        OUTCOME_TRY(batch->remove(encodeHeight(it->first)));
      }
      for (auto &[height, ts] : apply) {
        OUTCOME_TRY(batch->put(encodeHeight(height), encodeTsk(ts.key)));
      }
      OUTCOME_TRY(batch->commit());
    }

    forChild(branch, revert_to->first + 1, [&](auto &child) {
      child->chain.insert(revert_to, from.find(child->chain.begin()->first));
    });

    from.erase(revert_to, from.end());
    from.insert(apply.begin(), apply.end());
    return outcome::success();
  }

  outcome::result<TsBranchIter> find(TsBranchPtr branch,
                                     Height height,
                                     bool allow_less) {
    if (height > branch->chain.rbegin()->first) {
      return OutcomeError::kDefault;
    }
    TsBranchPtr parent;
    while (branch->chain.begin()->first > height) {
      if (!branch->parent) {
        return OutcomeError::kDefault;
      }
      parent = branch;
      branch = branch->parent;
    }
    auto it{branch->chain.lower_bound(height)};
    if (it == branch->chain.end()) {
      if (!allow_less) {
        // example: find 3 in [[1, 2], [4, 5]]
        if (!parent) {
          return OutcomeError::kDefault;
        }
        return std::make_pair(parent, parent->chain.begin());
      }
      --it;
    }
    if (it->first > height && allow_less) {
      if (it == branch->chain.begin()) {
        return OutcomeError::kDefault;
      }
      --it;
    }
    return std::make_pair(branch, it);
  }

  outcome::result<TsBranchIter> stepParent(TsBranchIter it) {
    auto &branch{it.first};
    if (it.second == branch->chain.begin()) {
      branch = branch->parent;
      if (!branch) {
        return OutcomeError::kDefault;
      }
      it.second = branch->chain.end();
    }
    --it.second;
    return it;
  }

  outcome::result<BeaconEntry> latestBeacon(TsLoadPtr ts_load,
                                            TsBranchPtr branch,
                                            Height height) {
    OUTCOME_TRY(it, find(branch, height));
    // magic number from lotus
    for (auto i{0}; i < 20; ++i) {
      OUTCOME_TRY(ts, ts_load->loadw(it.second->second));
      auto &beacons{ts->blks[0].beacon_entries};
      if (!beacons.empty()) {
        return *beacons.rbegin();
      }
      if (it.second->first == 0) {
        break;
      }
      OUTCOME_TRYA(it, stepParent(it));
    }
    return TipsetError::kNoBeacons;
  }
}  // namespace fc::primitives::tipset::chain
