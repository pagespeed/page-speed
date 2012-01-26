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

RELATIVE_PATH=$(dirname $0)

DEFAULT_NACL_HOME="${RELATIVE_PATH}/third_party/nacl_sdk"
DEFAULT_NACL_BUNDLE="pepper_17"

if [ -z "$NACL_HOME" ]; then NACL_HOME=$DEFAULT_NACL_HOME; fi
if [ -z "$NACL_BUNDLE" ]; then NACL_BUNDLE=$DEFAULT_NACL_BUNDLE; fi

if [ ! -d "$NACL_HOME" ]; then
  echo "Unable to find NACL_HOME at $NACL_HOME."
  exit 1
fi

./$NACL_HOME/naclsdk update $NACL_BUNDLE

NACL_BIN_DIR=$NACL_HOME/$NACL_BUNDLE/toolchain/linux_x86/bin
if [ ! -d "$NACL_BIN_DIR" ]; then
  echo "Failed to find NaCL bin dir."
  exit 1
fi

# Switch to the 64-bit toolchain if arch is x86_64.
if [ $(uname -m) = "x86_64" ]; then
  NACL_BIN_PREFIX="x86_64-nacl"
else
  NACL_BIN_PREFIX="i686-nacl"
fi


if [ ! -d "$NACL_BIN_DIR" ]; then
  echo "Unable to find NACL_BIN_DIR at $NACL_BIN_DIR."
  exit 1
fi

# Override the target toolchain used in the gyp-generated Makefile.
make CC.target=$NACL_BIN_DIR/$NACL_BIN_PREFIX-gcc         \
     CXX.target=$NACL_BIN_DIR/$NACL_BIN_PREFIX-g++        \
     LINK.target=$NACL_BIN_DIR/$NACL_BIN_PREFIX-g++       \
     AR.target=$NACL_BIN_DIR/$NACL_BIN_PREFIX-ar          \
     RANLIB.target=$NACL_BIN_DIR/$NACL_BIN_PREFIX-ranlib  \
     $@
