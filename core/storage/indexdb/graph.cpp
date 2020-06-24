/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graph.hpp"

#include "common/logger.hpp"

namespace fc::storage::indexdb {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("graph");
      return logger.get();
    }
  }  // namespace

  Branches Graph::getRoots() const {
    return getRootsOrHeads(roots_);
  }

  Branches Graph::getHeads() const {
    return getRootsOrHeads(heads_);
  }

  BranchId Graph::getLastBranchId() const {
    if (all_branches_.empty()) {
      return kNoBranch;
    }
    return all_branches_.rbegin()->first;
  }

  outcome::result<std::reference_wrapper<const BranchInfo>> Graph::getBranch(
      BranchId branchId) const {
    auto it = all_branches_.find(branchId);
    if (it == all_branches_.end()) {
      return Error::BRANCH_NOT_FOUND;
    }
    return it->second;
  }

  outcome::result<BranchId> Graph::findByHeight(Height height) const {
    if (current_chain_.empty()) {
      return Error::NO_CURRENT_CHAIN;
    }
    if (height < current_chain_bottom_height_) {
      return Error::BRANCH_NOT_FOUND;
    }
    auto it = current_chain_.lower_bound(height);
    if (it == current_chain_.end()) {
      return Error::BRANCH_NOT_FOUND;
    }
    return it->second;
  }

  outcome::result<void> Graph::load(
      std::map<BranchId, BranchInfo> all_branches) {
    clear();

    all_branches_ = std::move(all_branches);

    for (auto &[id, b] : all_branches_) {
      if (id != b.id || id == kNoBranch) {
        log()->error("cannot load graph: inconsistent branch id {}", id);
        return loadFailed();
      }

      if (b.top_height < b.bottom_height) {
        log()->error(
            "cannot load graph: heights inconsistent ({} and {}) for id {}",
            b.top_height,
            b.bottom_height,
            b.id);
        return loadFailed();
      }

      if (b.parent != kNoBranch) {
        if (b.parent == b.id) {
          log()->error(
              "cannot load graph: parent and branch id are the same ({})",
              b.id);
          return loadFailed();
        }
        auto it = all_branches_.find(b.parent);
        if (it == all_branches_.end()) {
          log()->error("cannot load graph: parent {} not found for branch {}",
                       b.parent,
                       b.id);
          return loadFailed();
        }

        auto &parent = it->second;

        if (parent.top_height >= b.bottom_height) {
          log()->error(
              "cannot load graph: parent height inconsistent ({} and {}) for "
              "id {} and parent {}",
              b.bottom_height,
              parent.top_height,
              b.id,
              b.parent);
          return loadFailed();
        }

        it->second.forks.insert(b.id);
      } else {
        roots_.insert(b.id);
      }
    }

    for (const auto &[_, b] : all_branches_) {
      if (b.forks.empty()) {
        heads_.insert(b.id);
      } else if (b.forks.size() == 1) {
        log()->warn("inconsistent # of forks (1) for branch {}, must be merged",
                    b.id);
      }
    }

    unsigned max_fork_height = all_branches_.size();

    for (auto root_id : roots_) {
      OUTCOME_TRY(assignRootFields(max_fork_height, root_id, 0, root_id));
    }

    return outcome::success();
  }

  BranchInfo &Graph::get(BranchId id) {
    assert(all_branches_.count(id) == 1);
    return all_branches_[id];
  }

  outcome::result<void> Graph::assignRootFields(unsigned max_fork_height,
                                                BranchId root_id,
                                                unsigned fork_height,
                                                BranchId id) {
    if (fork_height >= max_fork_height) {
      return Error::CYCLE_DETECTED;
    }

    // TODO recurse at the moment

    auto info = get(id);
    info.root = root_id;
    info.fork_height = fork_height;

    for (auto fork_id : info.forks) {
      OUTCOME_TRY(
          assignRootFields(max_fork_height, root_id, ++fork_height, fork_id));
    }

    return outcome::success();
  }

  outcome::result<void> Graph::switchToHead(BranchId head) {
    if (!current_chain_.empty() && current_chain_.rbegin()->second == head) {
      // we are already there, do nothing
      return outcome::success();
    }

    if (heads_.count(head) == 0) {
      log()->error("branch {} is not a head", head);
      return Error::BRANCH_IS_NOT_A_HEAD;
    }

    current_chain_.clear();

    // a guard to catch a cycle if it appears in the graph: db inconsistency
    size_t cycle_guard = all_branches_.size() + 1;
    BranchId curr_id = head;
    for (;;) {
      auto it = all_branches_.find(curr_id);

      assert(it != all_branches_.end());

      current_chain_[it->second.top_height] = current_chain_[it->second.id];
      curr_id = it->second.parent;
      if (curr_id == kNoBranch) {
        break;
      }

      if (--cycle_guard == 0) {
        current_chain_.clear();
        log()->error("cycle detected");
        return Error::CYCLE_DETECTED;
      }
    }

    current_chain_bottom_height_ =
        all_branches_[current_chain_.begin()->second].bottom_height;

    return outcome::success();
  }

  outcome::result<std::pair<BranchId, BranchId>> Graph::removeHead(
      BranchId head) {
    if (heads_.count(head) == 0) {
      log()->error("branch {} is not a head", head);
      return Error::BRANCH_IS_NOT_A_HEAD;
    }

    heads_.erase(head);
    roots_.erase(head);
    if (!current_chain_.empty() && current_chain_.rbegin()->second == head) {
      current_chain_.clear();
    }

    auto it = all_branches_.find(head);
    assert(it != all_branches_.end());

    auto parent = it->second.parent;

    all_branches_.erase(it);

    if (parent == kNoBranch) {
      return {kNoBranch, kNoBranch};
    }

    it = all_branches_.find(parent);
    assert(it != all_branches_.end());
    auto &pb = it->second;

    pb.forks.erase(head);
    if (pb.forks.size() != 1) {
      return {kNoBranch, kNoBranch};
    }

    // merge parent branch with its successor
    auto b = std::move(it->second);
    all_branches_.erase(it);
    return merge(b);
  }

  outcome::result<std::pair<BranchId, BranchId>> Graph::merge(BranchInfo &b) {
    BranchId successor_id = *b.forks.begin();
    auto s_it = all_branches_.find(successor_id);
    assert(s_it != all_branches_.end());
    auto &successor = s_it->second;
    successor.bottom = std::move(b.bottom);
    successor.bottom_height = b.bottom_height;
    successor.parent = b.parent;

    if (b.parent != kNoBranch) {
      auto p_it = all_branches_.find(b.parent);
      assert(p_it != all_branches_.end());
      auto &parent = p_it->second;
      parent.forks.erase(b.id);
      parent.forks.insert(successor_id);
    }

    return {b.parent, successor_id};
  }

  outcome::result<BranchId> Graph::linkBranches(BranchId base_branch,
                                                BranchId successor_branch,
                                                TipsetHash parent_tipset,
                                                Height parent_height) {
    // successor branch must be root
    if (roots_.count(successor_branch) == 0) {
      return Error::BRANCH_IS_NOT_A_ROOT;
    }

    // we are going to split base branch
    auto base_it = all_branches_.find(base_branch);
    if (base_it == all_branches_.end()) {
      return Error::BRANCH_NOT_FOUND;
    }
    // auto &base = base_it->second;

    // TODO
    return 0;
  }

  outcome::result<void> Graph::linkToHead(BranchId base_branch,
                                          BranchId successor_branch) {
    // successor branch must be root
    if (roots_.count(successor_branch) == 0) {
      return Error::BRANCH_IS_NOT_A_ROOT;
    }

    // base branch must be head
    if (heads_.count(base_branch) == 0) {
      return Error::BRANCH_IS_NOT_A_HEAD;
    }

    auto base_it = all_branches_.find(base_branch);
    if (base_it == all_branches_.end()) {
      return Error::BRANCH_NOT_FOUND;
    }
    auto &base = base_it->second;

    auto s_it = all_branches_.find(successor_branch);
    if (s_it == all_branches_.end()) {
      return Error::BRANCH_NOT_FOUND;
    }
    auto &successor = s_it->second;

    if (successor.bottom_height <= base.top_height) {
      return Error::LINK_HEIGHT_MISMATCH;
    }

    successor.bottom_height = base.bottom_height;
    successor.bottom = std::move(base.bottom);
    successor.parent = base.parent;

    if (base.parent != kNoBranch) {
      auto p_it = all_branches_.find(base.parent);
      assert(p_it != all_branches_.end());
      auto &parent = p_it->second;
      parent.forks.erase(base_branch);
      parent.forks.insert(successor_branch);
    } else {
      roots_.erase(base_branch);
      roots_.insert(successor_branch);
    }

    if (!current_chain_.empty()
        && current_chain_.rbegin()->second == base_branch) {
      current_chain_.clear();
    }

    heads_.erase(base_branch);

    // TODO all_branches_

    return outcome::success();
  }

  outcome::result<void> Graph::appendToHead(BranchId branch,
                                            TipsetHash new_top,
                                            Height new_top_height) {
    OUTCOME_TRY(b, getBranch(branch));
    const auto &branch_info = b.get();
    if (!branch_info.forks.empty()) {
      return Error::BRANCH_IS_NOT_A_HEAD;
    }

    if (new_top_height <= branch_info.top_height) {
      return Error::LINK_HEIGHT_MISMATCH;
    }

    // TODO this was already checked

    // current chain

    return outcome::success();
  }

  void Graph::clear() {
    all_branches_.clear();
    roots_.clear();
    heads_.clear();
    current_chain_.clear();
  }

  Branches Graph::getRootsOrHeads(const std::set<BranchId> &ids) const {
    Branches branches;
    branches.reserve(ids.size());
    for (const auto &id : ids) {
      assert(all_branches_.count(id));
      branches.push_back(all_branches_.at(id));
    }
    return branches;
  }

  outcome::result<void> Graph::loadFailed() {
    clear();
    return Error::GRAPH_LOAD_ERROR;
  }

}  // namespace fc::storage::indexdb
