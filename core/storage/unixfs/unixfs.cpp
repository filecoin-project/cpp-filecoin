/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/unixfs/unixfs.hpp"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <fstream>

#include "common/bytes_stream.hpp"
#include "common/error_text.hpp"
#include "common/file.hpp"
#include "crypto/hasher/hasher.hpp"
#include "storage/ipld/traverser.hpp"

namespace fc::storage::unixfs {
  using crypto::Hasher;
  using google::protobuf::io::CodedOutputStream;
  using google::protobuf::io::StringOutputStream;

  struct Wrap {
    Ipld &ipld;
    std::istream &stream;
    size_t remaining;
    size_t chunk_size;
    size_t max_links;
    Bytes chunk{};

    outcome::result<void> next() {
      chunk.resize(std::min(remaining, chunk_size));
      if (!common::read(stream, gsl::make_span(chunk))) {
        return ERROR_TEXT("FileSplit::next read error");
      }
      remaining -= chunk.size();
      return outcome::success();
    }
  };

  outcome::result<CID> makeLeaf(Wrap &w) {
    OUTCOME_TRY(w.next());
    CID cid{
        CID::Version::V1, CID::Multicodec::RAW, Hasher::blake2b_256(w.chunk)};
    OUTCOME_TRY(w.ipld.set(cid, w.chunk));
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

    Bytes toBytes() {
      cs.Trim();
      return Bytes(s.begin(), s.end());
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

  outcome::result<Tree> makeTree(Wrap &w, size_t height) {
    Tree root;
    PbFileBuilder pb_file;
    PbNodeBuilder pb_node;
    for (auto i = 0u; i < w.max_links && w.remaining != 0; ++i) {
      Tree tree;
      if (height == 1) {
        OUTCOME_TRYA(tree.cid, makeLeaf(w));
        tree.size = tree.file_size = w.chunk.size();
      } else {
        OUTCOME_TRYA(tree, makeTree(w, height - 1));
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
    OUTCOME_TRY(w.ipld.set(root.cid, node));
    return root;
  }

  outcome::result<CID> wrapFile(Wrap w) {
    size_t height = 0;
    for (auto max{w.chunk_size}; max < w.remaining; max *= w.max_links) {
      ++height;
    }
    if (height == 0) {
      return makeLeaf(w);
    }
    OUTCOME_TRY(tree, makeTree(w, height));
    return std::move(tree.cid);
  }

  outcome::result<CID> wrapFile(Ipld &ipld,
                                std::ifstream &file,
                                size_t chunk_size,
                                size_t max_links) {
    file.seekg(0, std::ios::end);
    const auto size{static_cast<size_t>(file.tellg())};
    file.seekg(0);
    if (!file.good()) {
      return ERROR_TEXT("wrapFile file error");
    }
    return wrapFile({ipld, file, size, chunk_size, max_links});
  }

  outcome::result<CID> wrapFile(Ipld &ipld,
                                gsl::span<const uint8_t> data,
                                size_t chunk_size,
                                size_t max_links) {
    BytesIstream stream{data};
    return wrapFile({ipld,
                     stream.s,
                     static_cast<size_t>(data.size()),
                     chunk_size,
                     max_links});
  }

  outcome::result<void> unwrapFile(std::ostream &file,
                                   Ipld &ipld,
                                   const CID &root) {
    ipld::traverser::Traverser traverser{ipld, root, {}, false};
    while (!traverser.isCompleted()) {
      OUTCOME_TRY(cid, traverser.advance());
      if (cid.content_type == CID::Multicodec::RAW) {
        OUTCOME_TRY(leaf, ipld.get(cid));
        if (!common::write(file, leaf)) {
          return ERROR_TEXT("unwrapFile write error");
        }
      } else if (cid.content_type != CID::Multicodec::DAG_PB) {
        return ERROR_TEXT("unwrapFile unexpected cid codec");
      }
    }
    return outcome::success();
  }
}  // namespace fc::storage::unixfs
