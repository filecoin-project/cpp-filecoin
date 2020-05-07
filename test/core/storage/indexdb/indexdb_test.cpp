/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <spdlog/fmt/fmt.h>
#include <boost/filesystem.hpp>

#include "storage/indexdb/indexdb.hpp"
#include "testutil/outcome.hpp"

using namespace fc::storage::indexdb;
namespace fs = boost::filesystem;

namespace {

  using BlobGraph = std::vector<std::vector<IndexDb::Blob>>;
  using CIDGraph = std::vector<std::vector<fc::CID>>;

  IndexDb::Blob createBlob(size_t n, int i) {
    uint64_t x = (n << 16) + i;
    auto p = (const uint8_t *)&x;
    return IndexDb::Blob(p, p + 8);
  }

  fc::CID createCID(size_t n, int i) {
    uint64_t x = (n << 16) + i;
    auto p = (const uint8_t *)&x;
    EXPECT_OUTCOME_TRUE_2(cid, fc::common::getCidOf(gsl::span(p, 8)));
    return cid;
  }

  template <class G>
  G createGraph(std::initializer_list<int> nodes_per_layers) {
    G g;
    g.reserve(nodes_per_layers.size());
    for (auto n : nodes_per_layers) {
      g.emplace_back();
      auto &v = g.back();
      v.reserve(n);
      for (int i = 0; i < n; ++i) {
        if constexpr(std::is_same<G, CIDGraph>::value) {
          v.push_back(createCID(g.size(), i));
        } else {
          v.push_back(createBlob(g.size(), i));
        }
      }
    }
    return g;
  }

  fc::outcome::result<void> setParent(IndexDb &db,
                                      const IndexDb::Blob &parent,
                                      const IndexDb::Blob &child) {
    return db.setParent(parent, child);
  }

  template <class G>
  void insertGraph(IndexDb &db, const G &g) {
    EXPECT_GE(g.size(), 2);

    auto tx = db.beginTx();

    for (auto n_layer = 0u; n_layer < g.size() - 1; ++n_layer) {
      const auto &parents = g[n_layer];
      const auto &children = g[n_layer + 1];
      for (const auto &p : parents) {
        for (const auto &c : children) {
          EXPECT_OUTCOME_TRUE_1(setParent(db, p, c));
        }
      }
    }

    tx.commit();
  }

  template <class C, class E>
  bool contains(const C &container, const E &element) {
    return std::find(container.begin(), container.end(), element)
           != container.end();
  }

}  // namespace

/**
 * @given
 * @when
 * @then
 */
TEST(IndexDb, Blobs_Graph) {
  EXPECT_OUTCOME_TRUE_2(db, createIndexDb(":memory:"));

  auto graph = createGraph<BlobGraph>({1, 2, 3, 4, 3, 2, 1});

  insertGraph(*db, graph);

  std::vector<IndexDb::Blob> blobs;
  EXPECT_OUTCOME_TRUE_1(db->getParents(
      graph[4][0], [&](const IndexDb::Blob &b) { blobs.push_back(b); }));

  EXPECT_EQ(blobs.size(), 4);
  for (auto &b : graph[3]) {
    EXPECT_TRUE(contains(blobs, b));
  }

  const auto &root = graph[0][0];
  auto from = graph.back()[0];
  bool root_found = false;
  for (auto i = 0u; i < graph.size(); ++i) {
    blobs.clear();
    EXPECT_OUTCOME_TRUE_1(db->getParents(from, [&](const IndexDb::Blob &b) {
      if (blobs.empty()) {
        blobs.push_back(b);
      }
      if (b == root) {
        root_found = true;
      }
    }));
    if (root_found) break;
    from = blobs[0];
  }
  EXPECT_TRUE(root_found);
}

/**
 * @given
 * @when
 * @then
 */
TEST(IndexDb, CIDs_Graph) {
  EXPECT_OUTCOME_TRUE_2(db, createIndexDb(":memory:"));

  auto graph = createGraph<CIDGraph>({1, 2, 3, 4, 3, 2, 1});

  insertGraph(*db, graph);

  EXPECT_OUTCOME_TRUE_2(cids, getParents(*db, graph[4][0]));

  EXPECT_EQ(cids.size(), 4);
  for (auto &c : graph[3]) {
    EXPECT_TRUE(contains(cids, c));
  }

  const auto &root = graph[0][0];
  auto from = graph.back()[0];
  bool root_found = false;
  for (auto i = 0u; i < graph.size(); ++i) {
    EXPECT_OUTCOME_TRUE_2(cc, getParents(*db, from));
    if (cc.empty()) {
      break;
    }
    if (contains(cc, root)) {
      root_found = true;
    }
    if (root_found) break;
    from = cc[0];
  }
  EXPECT_TRUE(root_found);
}
