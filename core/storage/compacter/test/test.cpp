#include <spdlog/sinks/basic_file_sink.h>

#include "storage/car/cids_index/util.hpp"
#include "storage/compacter/test/mut.hpp"
#include "storage/compacter/util.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "vm/interpreter/impl/cached_interpreter.hpp"
#include "vm/runtime/env.hpp"

const std::string kPath{"compacter-test-data"};
constexpr size_t kCompacterStateFull{10};
constexpr size_t kCompacterStateLookback{kCompacterStateFull + 20};
constexpr size_t kCompacterMessages{kCompacterStateFull};
constexpr size_t kMineSecp{50};
constexpr size_t kMineBls{200};

namespace fc {
  using boost::asio::io_context;
  using primitives::block::BlockHeader;
  using vm::state::StateTreeImpl;

  const auto io{std::make_shared<io_context>()};
  const auto ts_mutex{std::make_shared<std::shared_mutex>()};
  const auto ts_main{std::make_shared<TsBranch>()};
  const auto ts_branch{std::make_shared<TsBranch>()};
  const auto ts_branches{
      std::make_shared<TsBranches>(TsBranches{ts_main, ts_branch})};

  std::shared_ptr<storage::compacter::CompacterIpld> compacter;
  IpldPtr ipld;
  auto ipldBuf() {
    const auto buf{std::make_shared<vm::runtime::IpldBuffered>(ipld)};
    buf->actor_version = actorVersion(0);
    return buf;
  }

  size_t _receipt_id{0};
  CID makeReceipts(size_t n) {
    using vm::runtime::MessageReceipt;
    adt::Array<MessageReceipt> receipts{ipld};
    for (size_t i{0}; i < n; ++i) {
      O(receipts.append({{}, {}, (int64_t)_receipt_id++}));
    }
    return O(receipts.amt.flush());
  }
  namespace vm::interpreter {
    struct TestInterpreter : Interpreter {
      outcome::result<Result> interpret(TsBranchPtr ts_branch,
                                        const TipsetCPtr &ts) const override {
        auto ipld{ipldBuf()};
        const auto tree{
            std::make_shared<StateTreeImpl>(ipld, ts->getParentStateRoot())};
        mut::block(tree->getStore(), tree, ts->height());
        const auto state{O(tree->flush())};
        O(ipld->flush(state));
        const auto receipts{makeReceipts(kMineSecp + kMineBls)};
        return Result{state, receipts, {}};
      }
    };
  }  // namespace vm::interpreter

  size_t _message_id{0};
  CID makeMessages(size_t secp, size_t bls) {
    primitives::block::MsgMeta meta;
    cbor_blake::cbLoadT(ipld, meta);
    for (size_t i{0}; i < secp; ++i) {
      vm::message::SignedMessage smsg;
      smsg.message.nonce = _message_id++;
      O(meta.secp_messages.append(O(setCbor(ipld, smsg))));
    }
    for (size_t i{0}; i < bls; ++i) {
      vm::message::UnsignedMessage msg;
      msg.nonce = _message_id++;
      O(meta.bls_messages.append(O(setCbor(ipld, msg))));
    }
    return O(setCbor(ipld, meta));
  }
  auto pushBlock(const CbCid *parent,
                 ChainEpoch height,
                 const CID &state,
                 const CID &receipts,
                 const CID &messages) {
    BlockHeader block;
    block.ticket.emplace();
    if (parent) {
      block.parents.push_back(*parent);
    }
    block.height = height;
    block.parent_state_root = state;
    block.parent_message_receipts = receipts;
    block.messages = messages;
    const auto cbor{O(codec::cbor::encode(block))};
    const auto cid{CbCid::hash(cbor)};
    compacter->put_block_header->put(cid, BytesIn{cbor});

    return std::make_pair(block.height,
                          primitives::tipset::TsLazy{TipsetKey{{cid}}});
  }
  void mineGenesis() {
    std::shared_lock ts_lock{*ts_mutex};
    const auto tree{std::make_shared<StateTreeImpl>(ipld)};
    mut::genesis(tree->getStore(), tree);
    const auto genesis{pushBlock(
        nullptr, 0, O(tree->flush()), makeReceipts(0), makeMessages(0, 0))};
    ts_main->chain.emplace(genesis);
    ts_branch->chain.emplace(genesis);
  }
  void mineBlock() {
    auto parent{
        O(compacter->ts_load->load(ts_branch->chain.rbegin()->second.key))};
    auto result{O(compacter->interpreter->interpret(ts_branch, parent))};
    std::shared_lock ts_lock{*ts_mutex};

    if (ts_branch->chain.size() != 1) {
      ts_branch->chain.erase(ts_branch->chain.begin());
      ts_main->chain.emplace(*ts_branch->chain.begin());
    }

    const auto next{pushBlock(&parent->key.cids()[0],
                              parent->height() + 1,
                              result.state_root,
                              result.message_receipts,
                              makeMessages(kMineSecp, kMineBls))};

    ts_branch->chain.emplace(next);

    io->post(mineBlock);
  }
}  // namespace fc

int main() {
  using namespace fc;
  using storage::InMemoryStorage;

  setParams2K();  // actors v7

  boost::system::error_code ec;
  boost::filesystem::remove(kPath, ec);
  boost::filesystem::create_directories(kPath);

  common::file_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>(kPath + "/spd.log");
  spdlog::default_logger()->sinks().push_back(common::file_sink);

  const auto _ipld{*storage::cids_index::loadOrCreateWithProgress(
      kPath + "/ipld.car", true, boost::none, nullptr, nullptr)};

  compacter = storage::compacter::make(kPath + "/compacter",
                                       std::make_shared<InMemoryStorage>(),
                                       _ipld,
                                       ts_mutex);
  ipld = std::make_shared<CbAsAnyIpld>(compacter);
  ipld->actor_version = actorVersion(0);
  compacter->epochs_full_state = kCompacterStateFull;
  compacter->epochs_lookback_state = kCompacterStateLookback;
  compacter->epochs_messages = kCompacterMessages;
  compacter->ts_branches = ts_branches;
  compacter->ts_main = ts_main;
  compacter->interpreter_cache =
      std::make_shared<vm::interpreter::InterpreterCache>(
          std::make_shared<InMemoryStorage>(), compacter);
  compacter->interpreter->interpreter =
      std::make_shared<vm::interpreter::CachedInterpreter>(
          std::make_shared<vm::interpreter::TestInterpreter>(),
          compacter->interpreter_cache);
  compacter->ts_load = std::make_shared<primitives::tipset::TsLoadIpld>(ipld);

  mineGenesis();

  // loop compacter
  compacter->on_finish = [] { compacter->asyncStart(); };
  compacter->open();
  compacter->asyncStart();

  io->post(mineBlock);

  io->run();
}
