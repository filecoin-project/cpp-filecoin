/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipld/walker.hpp"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "common/todo_error.hpp"

namespace fc::storage::ipld::walker {
  using google::protobuf::io::CodedInputStream;
  using Input = gsl::span<const uint8_t>;

  struct PbDecoder {
    PbDecoder(Input input) : input{input}, cs(input.data(), input.size()) {}

    Input _str(uint64_t id) {
      if (cs.ExpectTag((id << 3) | 2)) {
        int size;
        if (cs.ReadVarintSizeAsInt(&size)) {
          auto offset = cs.CurrentPosition();
          if (cs.Skip(size)) {
            return input.subspan(offset, size);
          }
        }
      }
      return {};
    }

    bool empty() {
      return cs.CurrentPosition() >= input.size();
    }

    Input input;
    CodedInputStream cs;
  };

  struct PbNodeDecoder {
    static outcome::result<std::vector<CID>> links(
        gsl::span<const uint8_t> input) {
      std::vector<CID> cids;
      PbDecoder s{input};
      while (true) {
        PbDecoder link{s._str(2)};
        if (link.empty()) {
          break;
        }
        auto cid_bytes = link._str(1);
        if (cid_bytes.empty()) {
          return TodoError::ERROR;
        }
        OUTCOME_TRY(cid, CID::fromBytes(cid_bytes));
        cids.push_back(std::move(cid));
      }
      return std::move(cids);
    }
  };

  outcome::result<void> Walker::select(const CID &root,
                                       const Selector &selector) {
    // TODO(turuslan): implement selectors
    return recursiveAll(root);
  }

  outcome::result<void> Walker::recursiveAll(const CID &cid) {
    if (cids.insert(cid).second) {
      OUTCOME_TRY(bytes, store.get(cid));
      // TODO(turuslan): what about other types?
      if (cid.content_type == libp2p::multi::MulticodecType::DAG_CBOR) {
        try {
          CborDecodeStream s{bytes};
          recursiveAll(s);
        } catch (std::system_error &e) {
          return outcome::failure(e.code());
        }
      } else if (cid.content_type == libp2p::multi::MulticodecType::DAG_PB) {
        OUTCOME_TRY(cids, PbNodeDecoder::links(bytes));
        for (auto &cid : cids) {
          OUTCOME_TRY(recursiveAll(cid));
        }
      }
    }
    return outcome::success();
  }

  void Walker::recursiveAll(CborDecodeStream &s) {
    if (s.isCid()) {
      CID cid;
      s >> cid;
      auto result = recursiveAll(cid);
      if (!result) {
        outcome::raise(result.error());
      }
    } else if (s.isList()) {
      auto n = s.listLength();
      for (auto l = s.list(); n != 0; --n) {
        recursiveAll(l);
      }
    } else if (s.isMap()) {
      for (auto &p : s.map()) {
        recursiveAll(p.second);
      }
    } else {
      s.next();
    }
  }
}  // namespace fc::storage::ipld::walker
