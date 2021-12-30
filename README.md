# Filecoin (cpp-filecoin)

> C++17 implementation of blockchain based digital storage

Filecoin is a decentralized protocol described in [spec](https://filecoin-project.github.io/specs/)

## Minimal hardware requirements
### Node minimal parameters:
**Hard disk space**: at least 200 GB  
**RAM**: 8 GB  
**OS**: Linux(Ubuntu), macOS. Other operating systems builds are not supported yet, so running on them may be unstable.  
### Miner minimal parameters:
**CPU**: 8+ cores  
**Hard Disk space**: 256 GiB of very fast NVMe SSD memory space + 1 TiB of slow HDD memory space  
**RAM**: 16 GB  
**GPU**: GPU is highly recommended, it will speed up computations, but Mixing AMD CPUs and Nvidia GPUs should be avoided.

You can also read about lotus minimal requirements in [filecoin-docs](https://docs.filecoin.io/mine/hardware-requirements/#specific-operation-requirements "Minimal requirements filecoin-specific-configuration")
## Dependencies

All C++ dependencies are managed using [Hunter](https://github.com/cpp-pm/hunter).
It uses cmake to download required libraries and do not require downloading and installing packages manually.

Target C++ compilers are:
* GCC 9.3.0
* Clang 9.0.1
* AppleClang 12.0.0

### Lotus CLI
`fuhon-node` supports subset of `lotus` CLI commands.  
You may download [pre-built binaries](https://github.com/filecoin-project/lotus/releases) or build [lotus](https://github.com/filecoin-project/lotus) from source.  
Lotus CLI [introduction](https://docs.filecoin.io/get-started/lotus/installation/#interact-with-the-daemon).

### Rust
[filecoin-ffi](https://github.com/filecoin-project/filecoin-ffi) provides pre-built binaries for some platforms.  
If they are unavailable, you need Rust compiler to build them.  
Rust [installation instruction](https://www.rust-lang.org/tools/install).

## Build
```sh
# clone project
git clone --recursive https://github.com/filecoin-project/cpp-filecoin
# configure cmake
cmake cpp-filecoin -B cpp-filecoin/build
# build and install fuhon-node and fuhon-miner
cmake --build cpp-filecoin/build --target install
# check that fuhon-node and fuhon-miner are now available
fuhon-node --help
fuhon-miner --help
```

## Usage

### Interopnet node

Create the following `fuhon-interopnet/config.cfg` file
```properties
# use interopnet profile, corresponds to "make interopnet" lotus target
profile=interopnet

# enable debug logs to see sync progress
log=d

# bootstrap peers from https://github.com/filecoin-project/lotus/blob/master/build/bootstrap/interopnet.pi
bootstrap=/dns4/bootstrap-0.interop.fildev.network/tcp/1347/p2p/12D3KooWLGPq9JL1xwL6gHok7HSNxtK1Q5kyfg4Hk69ifRPghn4i
bootstrap=/dns4/bootstrap-1.interop.fildev.network/tcp/1347/p2p/12D3KooWFYS1f31zafv8mqqYu8U3hEqYvaZ6avWzYU3BmZdpyH3h
```

Start node
```sh
fuhon-node --repo fuhon-interopnet --genesis docker/mainnet/genesis.car
# you can omit --genesis flag after first run
fuhon-node --repo fuhon-interopnet
```

To use lotus CLI add `--repo` flag
```sh
lotus --repo fuhon-interopnet net peers
```

### Mainnet node (from snapshot)

Download mainnet snapshot ([docs](https://docs.filecoin.io/get-started/lotus/chain)).
```sh
LATEST_SNAPSHOT=$(curl -sI https://fil-chain-snapshots-fallback.s3.amazonaws.com/mainnet/minimal_finality_stateroots_latest.car | perl -ne '/x-amz-website-redirect-location:\s(.+\.car)/ && print $1')
curl -o mainnet-snapshot.car $LATEST_SNAPSHOT
```

Create following `fuhon-mainnet/config.cfg` file
```properties
# use downloaded snapshot file (do not delete that file)
use-snapshot=mainnet-snapshot.car

# bootstrap peers from https://github.com/filecoin-project/lotus/blob/master/build/bootstrap/mainnet.pi
bootstrap=/dns4/node.glif.io/tcp/1235/p2p/12D3KooWBF8cpp65hp2u9LK5mh19x67ftAam84z9LsfaquTDSBpt
```

Start node (first run may take some time)
```sh
fuhon-node --repo fuhon-mainnet --genesis cpp-filecoin/core/docker/mainnet/genesis.car
```

### Docker-compose example

```sh
docker-compose up
```

## CodeStyle

We follow [CppCoreGuidelines](https://github.com/isocpp/CppCoreGuidelines).

Please use clang-format 11.0.0 with provided [.clang-format](.clang-format) file to autoformat the code.

## Maintenance

Maintainers: @zuiris, @turuslan, @Elestrias, @ortyomka, @wer1st, @Alexey-N-Chernyshov

Tickets: Can be opened in GitHub Issues.

## Hunter cache upload

If you have access and want to upload to [hunter-binary-cache](https://github.com/soramitsu/hunter-binary-cache), you need to add your GitHub token with `read:packages` and `write:packages` permissions.  
To generate GitHub token follow the [instructions](https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token-for-the-command-line).
```sh
export GITHUB_HUNTER_USERNAME=<github account name>
export GITHUB_HUNTER_TOKEN=<github token>
```
