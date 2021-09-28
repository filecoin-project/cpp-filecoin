/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipld/traverser.hpp"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

namespace fc::storage::ipld::traverser {
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
        OUTCOME_TRY(cid, CID::fromBytes(cid_bytes));
        cids.push_back(std::move(cid));
      }
      return std::move(cids);
    }
  };

  Traverser::Traverser(Ipld &store,
                       const CID &root,
                       const Selector &selector,
                       bool unique)
      : store{store}, unique{unique} {
    to_visit_.push(root);
  }

  outcome::result<std::vector<CID>> Traverser::traverseAll() {
    // TODO(turuslan): implement selectors
    while (!isCompleted()) {
      OUTCOME_TRY(advance());
    }
    return visit_order_;
  }

  outcome::result<CID> Traverser::advance() {
    if (isCompleted()) {
      return TraverserError::kTraverseCompleted;
    }
    CID cid{std::move(to_visit_.front())};
    to_visit_.pop();
    if (unique) {
      visited_.insert(cid);
    }
    auto BOOST_OUTCOME_TRY_UNIQUE_NAME{gsl::finally([&] {
      if (unique) {
        while (!to_visit_.empty() && visited_.count(to_visit_.front()) != 0) {
          to_visit_.pop();
        }
      }
    })};
    OUTCOME_TRY(bytes, store.get(cid));
    visit_order_.push_back(cid);
    // TODO(turuslan): what about other types?
    if (cid.content_type == CID::Multicodec::DAG_CBOR) {
      CborDecodeStream s{bytes};
      OUTCOME_TRY(parseCbor(s));
    } else if (cid.content_type == CID::Multicodec::DAG_PB) {
      OUTCOME_TRY(cids, PbNodeDecoder::links(bytes));
      for (auto &&c : cids) {
        to_visit_.push(c);
      }
    }
    return cid;
  }

  bool Traverser::isCompleted() const {
    return to_visit_.empty();
  }

  outcome::result<void> Traverser::parseCbor(CborDecodeStream &s) {
    if (s.isCid()) {
      CID cid;
      s >> cid;
      to_visit_.push(cid);
    } else if (s.isList()) {
      auto n = s.listLength();
      for (auto l = s.list(); n != 0; --n) {
        OUTCOME_TRY(parseCbor(l));
      }
    } else if (s.isMap()) {
      for (auto &p : s.map()) {
        OUTCOME_TRY(parseCbor(p.second));
      }
    } else {
      s.next();
    }
    return outcome::success();
  }

}  // namespace fc::storage::ipld::traverser

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipld::traverser, TraverserError, e) {
  using fc::storage::ipld::traverser::TraverserError;

  switch (e) {
    case TraverserError::kTraverseCompleted:
      return "Traverser: blocks already completed";
    default:
      return "Traverser: unknown error";
  }
}
