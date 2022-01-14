/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/beast/http.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "primitives/jwt/jwt.hpp"

#include "drand/impl/http.cpp"
#include "common/uri_parser/uri_parser.hpp"
#include "primitives/piece/piece_data.hpp"
#include "sector_storage/impl/remote_worker.hpp"

namespace fc::sector_storage {
  using drand::http::ClientSession;
  using primitives::piece::MetaPieceData;
  using primitives::piece::ReaderType;
  namespace beast = boost::beast;
  namespace net = boost::asio;
  using tcp = net::ip::tcp;
  namespace uuids = boost::uuids;
  namespace http = boost::beast::http;
  using primitives::jwt::kAdminPermission;
  using common::HttpUri;


  outcome::result<std::shared_ptr<RemoteWorker>>
  RemoteWorker::connectRemoteWorker(io_context &context,
                                    const std::shared_ptr<CommonApi> &api,
                                    const std::string &address) {
    HttpUri uri;
    try {
      uri.parse(address);
    } catch (const std::runtime_error &err) {
      return sector_storage::stores::IndexErrors::kInvalidUrl;
    }

    OUTCOME_TRY(token, api->AuthNew({kAdminPermission}));

    struct make_unique_enabler : public RemoteWorker {
      explicit make_unique_enabler(io_context &context)
          : RemoteWorker{context} {};
    };

    std::unique_ptr<RemoteWorker> r_worker =
        std::make_unique<make_unique_enabler>(context);

    r_worker->port_ = std::to_string(uri.port());
    r_worker->host_ = uri.host();
    r_worker->wsc_.setup(r_worker->api_);

    OUTCOME_TRY(r_worker->wsc_.connect2(
        uri.host(), std::to_string(uri.port()), "/rpc/v0", std::string{token.begin(), token.end()}));

    return std::move(r_worker);
  }

  outcome::result<primitives::WorkerInfo> RemoteWorker::getInfo() {
    return api_.Info();
  }

  outcome::result<std::set<primitives::TaskType>>
  RemoteWorker::getSupportedTask() {
    return api_.TaskTypes();
  }

  outcome::result<std::vector<primitives::StoragePath>>
  RemoteWorker::getAccessiblePaths() {
    return api_.Paths();
  }

  RemoteWorker::RemoteWorker(io_context &context)
      : wsc_(*(worker_thread_.io)), io_(context) {} //TODO normaly

  struct PieceDataSender {
    explicit PieceDataSender(io_context &io)
        : resolver{io}, stream{net::make_strand(io)} {}
    PieceDataSender(const PieceDataSender &) = delete;
    PieceDataSender(PieceDataSender &&) = delete;
    ~PieceDataSender() {
      boost::system::error_code ec;
      stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    }
    PieceDataSender &operator=(const PieceDataSender &) = delete;
    PieceDataSender &operator=(PieceDataSender &&) = delete;

    static void send(
        int fd,
        io_context &io,
        const std::string &host,
        const std::string &port,
        const std::string &target,
        const std::uint64_t piece_size,
        const std::function<void(outcome::result<std::string>)> &cb) {
      auto s{std::make_shared<PieceDataSender>(io)};
      boost::system::error_code error;
      boost::beast::file_posix fp;
      fp.native_handle(fd);

      s->file_req.body().reset(std::move(fp), error);
      s->file_req.method(http::verb::post);
      s->file_req.target(target);
      s->file_req.set(http::field::host, host);
      s->file_req.set(http::field::content_length, piece_size);
      s->resolver.async_resolve(
          host, port, [s, MOVE(cb)](auto &&ec, auto &&iterator) {
            EC_CB();
            spdlog::info("ASYNC resolve successful");
            s->stream.async_connect(
                iterator, [s, MOVE(cb)](auto &&ec, auto &&) {
                  EC_CB();
                  spdlog::info("ASYNC connect successful");
                  http::async_write(s->stream,
                                    s->file_req,
                                    [s, MOVE(cb)](auto &&ec, auto &&) {
                                      EC_CB();
                                      spdlog::info("ASYNC Write successful");
                                      http::async_read(
                                          s->stream,
                                          s->buffer,
                                          s->res,
                                          [s, MOVE(cb)](auto &&ec, auto &&) {
                                            EC_CB();
                                            spdlog::info("HTTP SEND successful");
                                            cb(std::move(s->res.body()));
                                          });
                                    });
                });
          });
    }

   private:
    http::request<http::file_body> file_req;
    tcp::resolver resolver;
    beast::tcp_stream stream;
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
  };

  outcome::result<CallId> RemoteWorker::addPiece(
      const SectorRef &sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      PieceData piece_data) {
    MetaPieceData meta_data(uuids::to_string(uuids::random_generator()()),
                            ReaderType::Type::pushStreamReader);
    /* MetaPieceData meta_data("2032",
                            ReaderType::Type::nullReader); */
    //TODO(@Markuuusss) [FIL-560] Add Null PieceData to AddPiece.
    spdlog::info("Starting transfer piece data to remote worker");
    PieceDataSender::send(piece_data.release(),
                          *(httpSender.io),
                          host_,
                          port_,
                          "/rpc/streams/v0/push/" + meta_data.uuid,
                          new_piece_size,
                          [](const outcome::result<std::string> &res) {
                            std::cerr << res.value();
                          });
    spdlog::info("Transfer piece data to remote worker was successful");
  return api_.AddPiece(sector, piece_sizes, new_piece_size, meta_data);
  }

  outcome::result<CallId> RemoteWorker::sealPreCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      const std::vector<PieceInfo> &pieces) {
    return api_.SealPreCommit1(sector, ticket, pieces);
  }

  outcome::result<CallId> RemoteWorker::sealPreCommit2(
      const SectorRef &sector, const PreCommit1Output &pre_commit_1_output) {
    return api_.SealPreCommit2(sector, pre_commit_1_output);
  }

  outcome::result<CallId> RemoteWorker::sealCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      const InteractiveRandomness &seed,
      const std::vector<PieceInfo> &pieces,
      const SectorCids &cids) {
    return api_.SealCommit1(sector, ticket, seed, pieces, cids);
  }

  outcome::result<CallId> RemoteWorker::sealCommit2(
      const SectorRef &sector, const Commit1Output &commit_1_output) {
    return api_.SealCommit2(sector, commit_1_output);
  }

  outcome::result<CallId> RemoteWorker::finalizeSector(
      const SectorRef &sector, const gsl::span<const Range> &keep_unsealed) {
    return api_.FinalizeSector(
        sector, std::vector<Range>(keep_unsealed.begin(), keep_unsealed.end()));
  }

  outcome::result<CallId> RemoteWorker::moveStorage(const SectorRef &sector,
                                                    SectorFileType types) {
    return api_.MoveStorage(sector, types);
  }

  outcome::result<CallId> RemoteWorker::unsealPiece(
      const SectorRef &sector,
      UnpaddedByteIndex offset,
      const UnpaddedPieceSize &size,
      const SealRandomness &randomness,
      const CID &unsealed_cid) {
    return api_.UnsealPiece(sector, offset, size, randomness, unsealed_cid);
  }

  outcome::result<CallId> RemoteWorker::readPiece(
      PieceData output,
      const SectorRef &sector,
      UnpaddedByteIndex offset,
      const UnpaddedPieceSize &size) {
    return WorkerErrors::kUnsupportedCall;
  }

  outcome::result<CallId> RemoteWorker::fetch(const SectorRef &sector,
                                              const SectorFileType &file_type,
                                              PathType path_type,
                                              AcquireMode mode) {
    return api_.Fetch(sector, file_type, path_type, mode);
  }
}  // namespace fc::sector_storage
