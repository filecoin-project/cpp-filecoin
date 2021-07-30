/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/unixfs/unixfs.hpp"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "common/span.hpp"
#include "crypto/hasher/hasher.hpp"

namespace fc::storage::unixfs {
  using common::Buffer;
  using crypto::Hasher;
  using google::protobuf::io::CodedOutputStream;
  using google::protobuf::io::StringOutputStream;

  outcome::result<CID> makeLeaf(Ipld &ipld, gsl::span<const uint8_t> data) {
    CID cid{CID::Version::V1, CID::Multicodec::RAW, Hasher::blake2b_256(data)};
    OUTCOME_TRY(ipld.set(cid, Ipld::Value{data}));
    return cid;
  }

  struct PbBuilder {
    void _tag(uint64_t id, uint64_t value, bool str) {
      cs.WriteTag((id << 3) | (str ? 2 : 0));
      cs.WriteVarint64(value);
    }

    void _str(uint64_t id, gsl::span<const char> str) {
      _tag(id, str.size(), true);
      cs.WriteRaw(str.data(), str.size());
    }

    Buffer toBytes() {
      cs.Trim();
      return Buffer{common::span::cbytes(s)};
    }

    std::string toString() {
      cs.Trim();
      return s;
    }

    PbBuilder &operator+=(PbBuilder &other) {
      cs.Trim();
      s += other.toString();
      return *this;
    }

    std::string s;
    StringOutputStream ss{&s};
    CodedOutputStream cs{&ss};
  };

  struct PbNodeBuilder : PbBuilder {
    void link(CID cid, size_t size) {
      PbBuilder link;
      OUTCOME_EXCEPT(cid_bytes, cid.toBytes());
      link._str(1, common::span::cstring(cid_bytes));
      link._tag(2, 0, true);
      link._tag(3, size, false);
      _str(2, link.toString());
    }

    void content(gsl::span<const char> str) {
      _str(1, str);
    }
  };

  struct PbFileBuilder {
    void block(size_t size) {
      blocks._tag(4, size, false);
      total += size;
    }

    std::string toString() {
      PbBuilder file;
      file._tag(1, 2, false);
      file._tag(3, total, false);
      file += blocks;
      return file.toString();
    }

    size_t total{};
    PbBuilder blocks;
  };

  struct Tree {
    size_t size{}, file_size{};
    CID cid;
  };

  outcome::result<Tree> makeTree(Ipld &ipld,
                                 size_t height,
                                 gsl::span<const uint8_t> &data,
                                 size_t chunk_size,
                                 size_t max_links) {
    Tree root;
    PbFileBuilder pb_file;
    PbNodeBuilder pb_node;
    for (auto i = 0u; i < max_links && !data.empty(); ++i) {
      Tree tree;
      if (height == 1) {
        tree.size = tree.file_size =
            std::min(chunk_size, static_cast<size_t>(data.size()));
        OUTCOME_TRYA(tree.cid, makeLeaf(ipld, data.subspan(0, tree.file_size)));
        data = data.subspan(tree.file_size);
      } else {
        OUTCOME_TRYA(tree,
                     makeTree(ipld, height - 1, data, chunk_size, max_links));
      }
      root.size += tree.size;
      root.file_size += tree.file_size;
      pb_file.block(tree.file_size);
      pb_node.link(std::move(tree.cid), tree.size);
    }
    pb_node.content(pb_file.toString());
    auto node = pb_node.toBytes();
    root.size += node.size();
    root.cid =
        CID{CID::Version::V0, CID::Multicodec::DAG_PB, Hasher::sha2_256(node)};
    OUTCOME_TRY(ipld.set(root.cid, node));
    return root;
  }

  outcome::result<CID> wrapFile(Ipld &ipld,
                                gsl::span<const uint8_t> data,
                                size_t chunk_size,
                                size_t max_links) {
    size_t height = 0;
    for (ssize_t max = chunk_size; max < data.size(); max *= max_links) {
      ++height;
    }
    if (height == 0) {
      return makeLeaf(ipld, data);
    }
    OUTCOME_TRY(tree, makeTree(ipld, height, data, chunk_size, max_links));
    return std::move(tree.cid);
  }
}  // namespace fc::storage::unixfs
