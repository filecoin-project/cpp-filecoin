/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "api/full_node/node_api.hpp"
#include "api/full_node/node_api_v1_wrapper.hpp"
#include "api/rpc/client_setup.hpp"
#include "api/rpc/make.hpp"
#include "api/rpc/ws.hpp"
#include "api/rpc/wsc.hpp"
#include "common/io_thread.hpp"
#include "testutil/outcome.hpp"

namespace fc::api::full_node {
  using libp2p::multi::Multiaddress;

  class RpcApiTest : public ::testing::Test {
   public:
    void SetUp() override {
      std::stringstream ma_str;
      ma_str << "/ip4/127.0.0.1/tcp/" << api_port << "/http";
      multiaddress = Multiaddress::create(ma_str.str()).value();
    }

    void clientAsksVersion(const std::string &api_target,
                           const VersionResult expected_version) const {
      FullNodeApi client_api_v2;
      rpc::Client wsc_v2{*io.io};
      wsc_v2.setup(client_api_v2);
      EXPECT_OUTCOME_TRUE_1(wsc_v2.connect(multiaddress, api_target, ""));
      EXPECT_OUTCOME_EQ(client_api_v2.Version(), expected_version);
    }

    IoThread io;
    int api_port = 12345;
    Multiaddress multiaddress = common::kDefaultT<Multiaddress>();
  };

  /**
   * @given api v1 and v2 are provided with rpc web-socket server
   * @when client asks version with different targets
   * @then corresponding versions returned
   */
  TEST_F(RpcApiTest, Version) {
    FullNodeApi api_v2;
    api_v2.Version = []() {
      return VersionResult{"fuhon", makeApiVersion(2, 0, 0), 5};
    };
    auto api_v1 = makeFullNodeApiV1Wrapper();
    auto rpc_v1{makeRpc(api_v2)};
    wrapRpc(rpc_v1, *api_v1);
    auto rpc_v2{api::makeRpc(api_v2)};

    std::map<std::string, std::shared_ptr<api::Rpc>> rpcs;
    rpcs.emplace("/rpc/v0", rpc_v1);
    rpcs.emplace("/rpc/v1", rpc_v2);

    auto routes{std::make_shared<api::Routes>()};
    serve(rpcs, routes, *io.io, "127.0.0.1", api_port);

    clientAsksVersion("/rpc/v0", api_v1->Version().value());
    clientAsksVersion("/rpc/v1", {"fuhon", makeApiVersion(2, 0, 0), 5});
  }

  /**
   * @given server has v0 API
   * @when client connects with wrong version
   * @then error returned
   */
  TEST_F(RpcApiTest, WrongVersion) {
    std::map<std::string, std::shared_ptr<api::Rpc>> rpcs;
    rpcs.emplace("/rpc/v0", makeRpc(FullNodeApi{}));
    auto routes{std::make_shared<api::Routes>()};
    serve(rpcs, routes, *io.io, "127.0.0.1", api_port);

    FullNodeApi client_api;
    rpc::Client wsc{*io.io};
    wsc.setup(client_api);
    EXPECT_OUTCOME_FALSE_1(wsc.connect(multiaddress, "/rpc/wrong_version", ""));
  }

}  // namespace fc::api::full_node
