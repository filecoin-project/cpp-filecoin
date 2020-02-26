/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipld/impl/ipld_node_encoder_pb.hpp"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "ipld_node.pb.h"

using google::protobuf::io::ArrayOutputStream;
using google::protobuf::io::CodedOutputStream;
using protobuf::ipld::node::PBLink;
using protobuf::ipld::node::PBNode;

namespace fc::storage::ipld {
  common::Buffer IPLDNodeEncoderPB::encode(
      const common::Buffer &content,
      const std::map<std::string, IPLDLinkImpl> &links) {
    common::Buffer data;
    std::vector<uint8_t> links_pb = serializeLinks(links);
    std::vector<uint8_t> content_pb = serializeContent(content);
    data.put(links_pb);
    data.put(content_pb);
    return data;
  }

  size_t IPLDNodeEncoderPB::getLinkLengthPB(const std::string &name,
                                        const IPLDLinkImpl &link) {
    size_t length{};
    size_t cid_bytes_size = link.getCID().content_address.toBuffer().size();
    length += cid_bytes_size;
    length += CodedOutputStream::VarintSize64(cid_bytes_size);
    length += name.size();
    length += CodedOutputStream::VarintSize64(name.size());
    length += CodedOutputStream::VarintSize64(link.getSize());
    length += sizeof(PBTag) * 3;  // Count of the PBLink fields
    return length;
  }

  std::vector<uint8_t> IPLDNodeEncoderPB::serializeLinks(
      const std::map<std::string, IPLDLinkImpl> &links) {
    // Calculate links size:
    size_t links_content_size{};
    size_t links_headers_size{};
    std::vector<size_t> links_size{};
    for (const auto &link : links) {
      links_size.push_back(getLinkLengthPB(link.first, link.second));
      links_content_size += links_size.back();
      links_headers_size += sizeof(PBTag);
      links_headers_size += CodedOutputStream::VarintSize64(links_size.back());
    }
    if (links_content_size > 0) {
      std::vector<uint8_t> buffer(links_content_size + links_headers_size);
      ArrayOutputStream array_output_stream(buffer.data(), buffer.size());
      CodedOutputStream coded_stream{&array_output_stream};
      size_t link_index{};
      for (const auto &link : links) {
        PBTag links_tag = createTag(PBFieldType::LENGTH_DELEMITED,
                                    static_cast<uint8_t>(PBNodeOrder::LINKS));
        coded_stream.WriteTag(links_tag);
        coded_stream.WriteVarint64(links_size.at(link_index));
        // Write target Node's CID bytes:
        const auto &cid_bytes = link.second.getCID().content_address.toBuffer();
        PBTag cid_tag = createTag(PBFieldType::LENGTH_DELEMITED,
                                  static_cast<uint8_t>(PBLinkOrder::HASH));
        coded_stream.WriteTag(cid_tag);
        coded_stream.WriteVarint64(cid_bytes.size());
        coded_stream.WriteRaw(cid_bytes.data(), cid_bytes.size());
        // Write link name:
        PBTag name_tag = createTag(PBFieldType::LENGTH_DELEMITED,
                                   static_cast<uint8_t>(PBLinkOrder::NAME));
        coded_stream.WriteTag(name_tag);
        coded_stream.WriteVarint64(link.first.size());
        coded_stream.WriteRaw(link.first.data(), link.first.size());
        // Write target Node's size:
        PBTag size_tag = createTag(PBFieldType::VARINT,
                                   static_cast<uint8_t>(PBLinkOrder::SIZE));
        coded_stream.WriteTag(size_tag);
        coded_stream.WriteVarint64(link.second.getSize());
        ++link_index;
      }
      return buffer;
    }
    return {};
  }

  std::vector<uint8_t> IPLDNodeEncoderPB::serializeContent(
      const common::Buffer &content) {
    size_t pb_length = getContentLengthPB(content);
    std::vector<uint8_t> buffer(pb_length);
    if (pb_length > 0) {
      ArrayOutputStream array_output_stream(buffer.data(), buffer.size());
      PBTag data_tag = createTag(PBFieldType::LENGTH_DELEMITED,
                                 static_cast<uint8_t>(PBNodeOrder::DATA));
      CodedOutputStream coded_stream{&array_output_stream};
      coded_stream.WriteTag(data_tag);
      coded_stream.WriteVarint64(content.size());
      coded_stream.WriteRaw(content.data(), content.size());
    }
    return buffer;
  }

  size_t IPLDNodeEncoderPB::getContentLengthPB(const common::Buffer &content) {
    size_t length{};
    if (!content.empty()) {
      length += sizeof(PBTag);
      length += CodedOutputStream::VarintSize64(content.size());
      length += content.size();
    }
    return length;
  }

    IPLDNodeEncoderPB::PBTag IPLDNodeEncoderPB::createTag(PBFieldType type,
                                                uint8_t order) {
    constexpr size_t pb_type_length = 3;
    uint8_t tag = (order << pb_type_length);
    tag |= static_cast<uint8_t>(type);
    return tag;
  }
}  // namespace fc::storage::ipfs::merkledag
