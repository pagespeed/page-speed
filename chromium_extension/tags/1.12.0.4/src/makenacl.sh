#!/bin/bash
#
# Copyright 2010 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Allow callers to override the nacl home directory.
RELATIVE_PATH=$(dirname $0)
DEFAULT_NACL_HOME="${RELATIVE_PATH}/third_party/naclsdk"
if [ -z "$NACL_HOME" ]; then NACL_HOME=$DEFAULT_NACL_HOME; fi

if [ ! -d "$NACL_HOME" ]; then
  echo "Unable to find NACL_HOME at $NACL_HOME."
  exit 1
fi

# Use the 32-bit toolchain by default.
NACL_ARCH_DIR="nacl"

# Switch to the 64-bit toolchain if arch is x86_64.
if [ $(uname -m) = "x86_64" ]; then NACL_ARCH_DIR="nacl64"; fi

NACL_BIN=$NACL_HOME/toolchain/linux_x86/$NACL_ARCH_DIR/bin

if [ ! -d "$NACL_BIN" ]; then
  echo "Unable to find NACL_BIN at $NACL_BIN."
  exit 1
fi

# Override the target toolchain used in the gyp-generated Makefile.
make CC.target=$NACL_BIN/gcc         \
     CXX.target=$NACL_BIN/g++        \
     LINK.target=$NACL_BIN/g++       \
     AR.target=$NACL_BIN/ar          \
     RANLIB.target=$NACL_BIN/ranlib  \
     $@
