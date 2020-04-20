#!/usr/bin/env bash
# shellcheck disable=SC2155 enable=require-variable-braces

set -Eeo pipefail

# tracks where the Rust sources are were we to build locally instead of
# downloading from GitHub Releases
#
rust_sources_dir="../filecoin-ffi/rust"
release_sha1=$(cd $rust_sources_dir && git rev-parse HEAD)
if [ -e filecoin_ffi_commit_installed ] && [ "$(cat filecoin_ffi_commit_installed)" = "$release_sha1" ]; then
  (>&2 echo "filecoin-ffi is already installed")
  exit 0
fi

auth_header=()
if [ -n "${GITHUB_TOKEN}" ]; then
	auth_header=("-H" "Authorization: token ${GITHUB_TOKEN}")
fi

# set CWD to the root of filecoin-ffi
#
cd "$(dirname "${BASH_SOURCE[0]}")"

if ! [ -x "$(command -v jq)" ]; then
  (>&2 echo 'Error: jq is not installed.')
  (>&2 echo 'Install jq to resolve this problem.')
  exit 1
fi

# an array of values passed as 'target-feature' to the Rust compiler if we're
# building an optimized libfilcrypto (which takes advantage of some perf-boosting
# instruction sets)
#
optimized_release_rustc_target_features=$(jq -r '.[].rustc_target_feature' < "${rust_sources_dir}/rustc-target-features-optimized.json")

# each value in this area is checked against the "features" of the hosts CPU
# in order to determine if the host is suitable for an optimized release
#
cpu_features_required_for_optimized_release=$(jq -r '.[].check_cpu_for_feature | select(. != null)' < "${rust_sources_dir}/rustc-target-features-optimized.json")

main() {
    mkdir -p include/filecoin-ffi
    mkdir -p lib/pkgconfig
    local __release_type=$(get_release_type)
    if [ "${FFI_BUILD_FROM_SOURCE}" != "1" ] && download_release_tarball __tarball_path "${rust_sources_dir}" "filecoin-ffi" "${__release_type}"; then
        local __tmp_dir=$(mktemp -d)

        # silence shellcheck warning as the assignment happened in
        # `download_release_tarball()`
        # shellcheck disable=SC2154
        # extract downloaded tarball to temporary directory
        #
        tar -C "${__tmp_dir}" -xzf "${__tarball_path}"

        # copy build assets into root of filecoin-ffi
        #
        find -L "${__tmp_dir}" -type f -name filcrypto.h -exec rsync --checksum "{}" ./include/filecoin-ffi \;
        find -L "${__tmp_dir}" -type f -name libfilcrypto.a -exec rsync --checksum "{}" ./lib \;
        find -L "${__tmp_dir}" -type f -name filcrypto.pc -exec rsync --checksum "{}" ./lib/pkgconfig \;

        check_installed_files

        (>&2 echo "[install-filcrypto/main] successfully installed prebuilt libfilcrypto")
    else
        (>&2 echo "[install-filcrypto/main] building libfilcrypto from local sources (dir = ${rust_sources_dir})")

        # build libfilcrypto (and corresponding header and pkg-config)
        #
        build_from_source "filcrypto" "${rust_sources_dir}" "${__release_type}"

        # copy from Rust's build directory (target) to root of filecoin-ffi
        #
        find -L "${rust_sources_dir}/target/release" -type f -name filcrypto.h -exec rsync --checksum "{}" ./include/filecoin-ffi \;
        find -L "${rust_sources_dir}/target/release" -type f -name libfilcrypto.a -exec rsync --checksum "{}" ./lib \;
        find -L "${rust_sources_dir}" -type f -name filcrypto.pc -exec cp rsync --checksum "{}" ./lib/pkgconfig \;

        check_installed_files

        (>&2 echo "[install-filcrypto/main] successfully built and installed libfilcrypto from source")
    fi
    echo "$release_sha1" > filecoin_ffi_commit_installed
}

download_release_tarball() {
    local __resultvar=$1
    local __rust_sources_path=$2
    local __repo_name=$3
    local __release_type=$4
    local __release_tag="${release_sha1:0:16}"
    local __release_tag_url="https://api.github.com/repos/filecoin-project/${__repo_name}/releases/tags/${__release_tag}"

    # TODO: This function shouldn't make assumptions about how these releases'
    # names are constructed. Marginally less-bad would be to require that this
    # function's caller provide the release name.
    #
    local __release_name="${__repo_name}-$(uname)-${__release_type}"

    (>&2 echo "[download_release_tarball] acquiring release @ ${__release_tag}")

    local __release_response=$(curl "${auth_header[@]}" \
        --retry 3 \
        --location "${__release_tag_url}")

    local __release_url=$(echo "${__release_response}" | jq -r ".assets[] | select(.name | contains(\"${__release_name}\")) | .url")

    local __tar_path="/tmp/${__release_name}_$(basename "${__release_url}").tar.gz"

    if [[ -z "${__release_url}" ]]; then
        (>&2 echo "[download_release_tarball] failed to download release (tag URL: ${__release_tag_url}, response: ${__release_response})")
        return 1
    fi

    local __asset_url=$(curl "${auth_header[@]}" \
        --head \
        --retry 3 \
        --header "Accept:application/octet-stream" \
        --location \
        --output /dev/null \
        -w "%{url_effective}" \
        "${__release_url}")

    if ! curl --retry 3 --output "${__tar_path}" "${__asset_url}"; then
        (>&2 echo "[download_release_tarball] failed to download release asset (tag URL: ${__release_tag_url}, asset URL: ${__asset_url})")
        return 1
    fi

    # set $__resultvar (which the caller provided as $1), which is the poor
    # man's way of returning a value from a function in Bash
    #
    eval "${__resultvar}='${__tar_path}'"
}

build_from_source() {
    local __library_name=$1
    local __rust_sources_path=$2
    local __release_type=$3
    local __repo_sha1_truncated="${release_sha1:0:16}"
    local __target_feature=""

    (>&2 echo "building from source @ ${__repo_sha1_truncated}")

    if ! [ -x "$(command -v cargo)" ]; then
        (>&2 echo '[build_from_source] Error: cargo is not installed.')
        (>&2 echo '[build_from_source] install Rust toolchain to resolve this problem.')
        exit 1
    fi

    if ! [ -x "$(command -v rustup)" ]; then
        (>&2 echo '[build_from_source] Error: rustup is not installed.')
        (>&2 echo '[build_from_source] install Rust toolchain installer to resolve this problem.')
        exit 1
    fi

    pushd "${__rust_sources_path}"

    cargo --version

    # reduce array of features into the rustc-specific 'target-feature' flag
    # shellcheck disable=SC2068 # the splitting is intentional
    for x in ${optimized_release_rustc_target_features[@]}; do
        __target_feature="${x},${__target_feature}"
    done

    if [ "${__release_type}" = "optimized" ]; then
        RUSTFLAGS="-C target-feature=${__target_feature}" ./scripts/build-release.sh "${__library_name}" "$(cat rust-toolchain)"
    else
        ./scripts/build-release.sh "${__library_name}" "$(cat rust-toolchain)"
    fi

    popd
}

get_release_type() {
    local __searched=""
    local __features=""
    local __optimized=true

    # determine where to look for CPU features
    #
    if [[ ! -f "/proc/cpuinfo" ]]; then
        (>&2 echo "[get_release_type] no /proc/cpuinfo file; falling back to Darwin feature detection")
        __searched="sysctl -a | grep machdep.cpu | tr '[:upper:]' '[:lower:]' | grep features"
    else
        __searched="cat /proc/cpuinfo | grep flags"
    fi
    __features=$(eval "${__searched}")

    # check for the presence of each required CPU feature
    #
    # shellcheck disable=SC2068 # the splitting is intentional
    for x in ${cpu_features_required_for_optimized_release[@]}; do
        if [ "${__optimized}" = true ]; then
            if [ -n "${__features##*${x}*}" ]; then
                (>&2 echo "[get_release_type] your CPU does not support the '${x}' feature (searched '${__searched}')")
                __optimized=false
            fi
        fi
    done

    # if we couldn't figure out where to look for features, use standard
    #
    if [ "${__optimized}" == true ] && [ -n "${__features}" ]; then
        (>&2 echo "[get_release_type] configuring 'optimized' build")
        echo "optimized"
    else
        (>&2 echo "[get_release_type] configuring 'standard' build")
        echo "standard"
    fi
}

check_installed_files() {
    if [[ ! -f "./include/filecoin-ffi/filcrypto.h" ]]; then
        (>&2 echo "[check_installed_files] failed to install filcrypto.h")
        exit 1
    fi

    if [[ ! -f "./lib/libfilcrypto.a" ]]; then
        (>&2 echo "[check_installed_files] failed to install libfilcrypto.a")
        exit 1
    fi

    if [[ ! -f "./lib/pkgconfig/filcrypto.pc" ]]; then
        (>&2 echo "[check_installed_files] failed to install filcrypto.pc")
        exit 1
    fi
}

main "$@"; exit
