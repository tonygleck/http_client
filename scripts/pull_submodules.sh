#!/bin/bash

# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

script_dir=$(cd "$(dirname "$0")" && pwd)
root_dir=$(cd "${script_dir}/.." && pwd)

# update lib-util-c
lib_util_dir=$root_dir"/deps/lib-util-c"
pushd $lib_util_dir
git checkout master
git pull origin master
popd

patchcord_dir=$root_dir"/deps/patchcords"
pushd $patchcord_dir
git checkout master
git pull origin master
popd
: