# Copyright 2012 Google Inc.
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

vars = {
  "chromium_trunk": "http://src.chromium.org/svn/trunk",
  "chromium_revision": "@90205",
  "chromium_deps_root": "src/third_party/chromium_deps",
  "modpagespeed_src": "http://modpagespeed.googlecode.com/svn/trunk",
  "modpagespeed_revision": "@2380",
  "libharu_trunk": "git://github.com/libharu/libharu.git",
  "libharu_revision": "@RELEASE_2_2_0",
  "protobuf_trunk": "http://protobuf.googlecode.com/svn/tags/2.4.1",
  "protobuf_revision": "@head",
  "drp_src": "http://domain-registry-provider.googlecode.com/svn/trunk",
  "drp_revision": "@29",
}

deps = {
  # Fetch chromium DEPS so we can sync our other dependencies relative
  # to it.
  Var("chromium_deps_root"):
    File(Var("chromium_trunk") + "/src/DEPS" + Var("chromium_revision")),

  "src/testing":
    Var("chromium_trunk") + "/src/testing" + Var("chromium_revision"),

  "src/third_party/chromium/src/build":
    Var("chromium_trunk") + "/src/build" + Var("chromium_revision"),

  "src/third_party/chromium/src/base":
    Var("chromium_trunk") + "/src/base" + Var("chromium_revision"),

  "src/build/linux":
    Var("chromium_trunk") + "/src/build/linux" + Var("chromium_revision"),
  "src/build/mac":
    Var("chromium_trunk") + "/src/build/mac" + Var("chromium_revision"),
  "src/build/win":
    Var("chromium_trunk") + "/src/build/win" + Var("chromium_revision"),
  "src/build/util":
    Var("chromium_trunk") + "/src/build/util" + Var("chromium_revision"),
  "src/build/internal":
    Var("chromium_trunk") + "/src/build/internal" + Var("chromium_revision"),

  "src/third_party/domain_registry_provider/src":
    Var("drp_src") + "/src" + Var("drp_revision"),

  "src/third_party/libharu/libharu":
    Var("libharu_trunk") + Var("libharu_revision"),

  "src/third_party/modp_b64":
    Var("chromium_trunk") + "/src/third_party/modp_b64" +
      Var("chromium_revision"),

  "src/third_party/protobuf":
    (Var("chromium_trunk") + "/src/third_party/protobuf" +
     Var("chromium_revision")),

  "src/third_party/protobuf/java":
    (Var("protobuf_trunk") + "/java/src/main/java" +
     Var("protobuf_revision")),

  "src/third_party/protobuf/java/descriptor":
    File(Var("protobuf_trunk") + "/src/google/protobuf/descriptor.proto" +
     Var("protobuf_revision")),

  "src/tools/data_pack":
    Var("chromium_trunk") + "/src/tools/data_pack" + Var("chromium_revision"),

  "src/tools/grit":
    Var("chromium_trunk") + "/src/tools/grit" + Var("chromium_revision"),

  "src/googleurl": From(Var("chromium_deps_root")),

  "src/net/instaweb":
    Var("modpagespeed_src") + "/src/net/instaweb" + Var("modpagespeed_revision"),

  "src/testing/gmock": From(Var("chromium_deps_root")),
  "src/testing/gtest": From(Var("chromium_deps_root")),
  "src/tools/gyp": From(Var("chromium_deps_root")),

  "src/third_party/gflags": "/deps/gflags-2.0",
  "src/third_party/icu": "/deps/icu461",
  "src/third_party/libjpeg": "/deps/jpeg-8c",
  "src/third_party/libpng": "/deps/libpng-1.5.4",
  "src/third_party/libwebp": "/deps/libwebp-0.2.1",
  "src/third_party/nacl_sdk": "/deps/nacl_sdk",
  "src/third_party/zlib": "/deps/zlib-1.2.5",
}


deps_os = {
  "win": {
    "src/third_party/cygwin": From(Var("chromium_deps_root")),
    "src/third_party/python_26": From(Var("chromium_deps_root")),
  },
  "mac": {
  },
  "unix": {
  },
}


include_rules = [
  # Everybody can use some things.
  "+base",
  "+build",
]


# checkdeps.py shouldn't check include paths for files in these dirs:
skip_child_includes = [
   "testing",
]


hooks = [
  {
    # A change to a .gyp, .gypi, or to GYP itself should run the generator.
    "pattern": ".",
    "action": ["python", "src/build/gyp_chromium"],
  },
]
