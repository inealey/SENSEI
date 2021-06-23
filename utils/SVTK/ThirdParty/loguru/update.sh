#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="loguru"
readonly ownership="loguru Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/svtk$name"
readonly repo="https://gitlab.kitware.com/third-party/loguru.git"
readonly tag="for/svtk"
readonly paths="
CMakeLists.svtk.txt
.gitattributes
LICENSE
loguru.cpp
loguru.hpp
README.kitware.md
README.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.svtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
