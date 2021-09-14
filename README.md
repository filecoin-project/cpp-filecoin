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
* GCC 7.4
* Clang 6.0.1
* AppleClang 11.0

### Rust Installation

C++ Filecoin implementation uses Rust libraries as dependencies for BLS provider.
All you need to build these targets successfully - is Rust lang installed to your `${HOME}/.cargo/bin`.

You can do it manually following the instructions from [rust-lang.org](https://www.rust-lang.org/tools/install) or any other way.

Otherwise, CMake would not be able to compile Rust-related targets and will produce a warning about that during configuration.

### CodeStyle

We follow [CppCoreGuidelines](https://github.com/isocpp/CppCoreGuidelines).

Please use clang-format 11.0.0 with provided [.clang-format](.clang-format) file to autoformat the code.

## Maintenance

Maintainers: @zuiris, @turuslan, @Elestrias, @ortyomka, @wer1st, @Alexey-N-Chernyshov

Tickets: Can be opened in GitHub Issues.


# Cpp-Filecoin install and setup guide
## Fuhon node establishing
There are 2 ways of getting executable version of filecoin node and miner. The First is to get executables by downloading, you can do it here: `https://link-tofiles`.The second is to build executables by yourself, see **Advanced building**. To begin running a node you will need `node` executable.

1. To configure node you also need to make:
* **configuration file**
  example `config.cfg`:

     ```
     bootstrap=<enter your peer here>
     drand-server=api.drand.sh
     drand-pubkey=868f005eb8e6e4ca0a47c8a77ceaa5309a47978a7c71bc5cce96366b5d7a569937c529eeda66c7293784a9402801af31
     drand-genesis-time=1595431050
     drand-period=30
     ```
* **genesisfile** - this is a special pre-set file with the initial block, which will configure the initial state of your node, set network parameters like sector size, recall time, e.t.c and define miner-list with first network miner.

you also can download that [snapshots](https://fil-chain-snapshots-fallback.s3.amazonaws.com/mainnet/minimal_finality_stateroots_latest.car "snapshots store") for a less time-assuming connection and synchronisation to the popular filecoin networks.
More about snapshot management [here](https://docs.filecoin.io/get-started/lotus/chain/#restoring-a-custom-snapshot "snapshot documentation").
2. After all the preparations you can launch a node:
*  link working directory, which will be a storage for all node-associated files. It can be done with `--repo` flag
*  One more important linking is configuration and genesis files, you can pass paths to them with `--config CONFIG_FILE` and `--genesis GENESIS_FILE` respectively.
*  For specifying peers listening port you need to pass `--port PORT` flag
*  In case you want to specify api port for a node + miner configuration or multi-node configuration,  you can do that with `--api API_PORT`
*  Example of the command: `node_exec --repo path_to_node_repo --genesis genesis.car --config config.cfg  --port 8080 --api 1235`

3. Once the node has been successfully configured, it will start to connect to network peers and download blockchain history. This process may continue for a while(f.e. the weight of chain of an interopnet network is 3.5 GB). As it takes a while, you can change cids_index.car file to downloaded snapshot(see step 1 for that).

## Lotus-CLI establishing
### User approach:
1. Download [lotus executable](https://github.com/filecoin-project/lotus/releases/ "lotus releases")
2. You can use CLI client with your node by linking it with `--repo <path to your node working directory>` flag,
   more of lotus-CLI flags will be covered in **Work with Lotus-CLI**

### Advanced approach:
1. clone [lotus repository](https://github.com/filecoin-project/lotus "lotus project")
2. build lotus cli client with command: `make build`
3. In case you want to connect to interopnet, build lotus with `make interopnet`
## Work with Lotus-CLI

1. For proper miner work you will need a bls wallet, which can be made with lotus-cli by using the following command: `$lotus --repo REPO wallet new bls`, after the execution of this command you will revive the public address of your new account, and a private key in `<repo of youe working node>/keystore`.
2. Also, you may need to put some Filecoins on your account from other addresses you may have, and you can do it  with `lotus --repo REPO send --from SENDER_ADDRESS DISTANATION_ADDRESS`

3. In case you want to get some Filecoins, you can buy them on stocks with usage of you public address.

4. Other advanced commands you may find in [filecoin-docs about lotus CLI](https://docs.filecoin.io/mine/lotus/dagstore/#cli-commands "lotus CLI documentation").
## Fuhon miner establishing
To start mining at first you need to configure and run Fuhon or Lotus node. Before all make sure that your node had successfully **connected** and updated to the current **block height** (actual network block number). Also, your node's api port should be opened for connection.

To provide wallet functions and oracle information be sure to run lotus-cli at first. You can find how to do it in **Lotus-CLI establishing**.

Once all the needed  modules will be started, you can continue with miner configuration.

1. Create miner work directory (f.e. `$mkdir miner`)
2. Download miner executable here : `https://link_to_download` and put it in work directory
   3.Copy proof parameters json file with command: `cp /<path to cpp-filecoin>/core/proofs/parameters.json MINER_REPO/proof-params.json`
4. Run yor miner with ` --repo REPO --miner-repo MINER_REPO --owner PUBLIC_ADRESS --miner-api API_PORT --sector-size SECTOR_SIZE`

## Advanced Building
This approach allows to build our project with your modifications,
but be sure that your realisation follows consensus rules.

1. clone [funon-filecoin project](https://github.com/filecoin-project/cpp-filecoin "cpp-filecoin project")

2. go to `./cpp-filecoin` directory

3. run `$git submodule --recursive --update`

4. To finish your building you may also want to add your GitHub token, In such case you need to set up two environment variables:
```
GITHUB_HUNTER_USERNAME=<github account name>
GITHUB_HUNTER_TOKEN=<github token>
```
To generate GitHub token follow the [instructions](https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token-for-the-command-line). Make sure `read:packages` and `write:packages` permissions are granted.

5. At this stage you will need a build directory, which can be created with `mkdir BUILD_REPO`
   
6. Build cmake with the following command: `cmake . -B BUILD_DIR`

7. build targets: node - for node executable, miner_main- for miner executable.
   example: `cmake --build BUILD_DIR --target node miner_main`

8. First build will likely take long time. However, you can cache binaries to [hunter-binary-cache].

9. It is also suggested to build project with clang-tidy checks, however if you wish to omit clang-tidy step, you can use `cmake ..` instead.

- Tests can be run with:
  ```
  cd build
  ctest
  ```

10. node executable can be found at: `<build_repository>/core/node/main`.
11. miner executable can be found at:
   `<build_repository>/core/miner/main`
12. next steps are similar to common miner and node establishing which can be found at **Fuhon node establishing** and **Fuhon miner establishing**

## Configuration Lotus-Fuhon
### Lotus-node and Fuhon-miner
1. To start up a lotus node at first you need to get [lotus executable](https://github.com/filecoin-project/lotus/releases/ "lotus releases")
2. You can run your lotus node with the next command: `$lotus --repo REPO daemon --genesis genesis.car --profile bootstrapper`, more about running lotus software can be found in [filecoin-docs](https://docs.filecoin.io/get-started/lotus/installation "lotus filecoin documentation").

### Fuhon-node and Lotus-miner
1. Establishing of a Fuhon node is described in **Fuhon miner establishing**
2. Lotus miner can be established by the following way:
   - to begin with you need you own bls wallet(described in **Working with Lotus-CLI**)
   - init your miner with command: `lotus-miner --repo REPO --miner-repo MINER_REPO init --nosync --owner BLS_WALLET_ADDR --sector-size SECTOR_SIZE`
   - to know more about lotus software establishing, read [filecoin-docs](https://docs.filecoin.io/get-started/lotus/installation/#minimal-requirements "lotus establishing guide")

## Docker configuration
    Will be soon!

## Your new Filecoin network start

To run a new filecoin network, you should run at least one node, and a miner (miners are representation of the network power, so more miners connected and sealed, more power your network will have). In this guide we will consider network of two nodes and two miners(genesis and common ones)

1. To run the first node you will need to generate a new genesis file which will be an initial instruction for all new connected nodes.
   This can be done with lotus-seed tools:
   - Clone  [lotus repository](https://github.com/filecoin-project/lotus "lotus project")

   - build lotus-seed with `$make lotus-seed` or `make debug` - for debug version initiation,
     or `make 2k` for version with 2 KiB sector size  or `make interopnet` for interopnet version.

   - Generate genesis and preseal. Ensure lotus-seed was build in previous step and run script node `$genesis.js DIR 1` to generate genesis. `DIR` - directory where genesis and presealed sectors will be generated, 1 - number of sectors for miner. In case your lotus executables lay in custom directory or script can not find them, you can link lotus target with `LOTUS=/<path to your lotus executable>/lotus node genesis.js <directory where results wil be> <sealing sectors upper bound>`

2. Run your node-1 with new genesis file as it was done in chapter **Fuhon node establishing**

3. Run your node-2 as the 1st one, but be sure to open api port on it with `--api API_PORT`

4. Connect node-2 to node-1 by commands:
   `addr=$(lotus --repo REPO1 net listen | grep 127.0.0.1)`
   `lotus --repo REPO2 net connect $addr`t

5. After generating new genesis file, you will also receive genesis.json with declared miners list, these are addresses of the initial miners.

6. To start miner that is declared in genesisfile (step 5) you can use chapter **Fuhon miner establishing** but with some major changes.
   You will also need the following flags to be provided:   `--genesis-miner`, `--actor ACTOR`, `--pre-sealed-sectors PRESEAL_DIR`, `--pre-sealed-metadata PRESEAL_JSON`.

7. To start new miner just follow the steps declared in **Fuhon miner establishing**, or you can run lotus miner using [filecoin-docs](https://docs.filecoin.io/get-started/lotus/installation/#minimal-requirements "lotus establishing guide")
8. After two nodes will be connected and miner will work, your new filecoin network starting to exist, since that moment you can share your peers to invite new participants. 