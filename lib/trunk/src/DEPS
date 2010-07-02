# Copyright 2009 Google Inc.
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
  "chromium_trunk":
    "http://src.chromium.org/svn/trunk",
  "chromium_revision": "@50096",
  "chromium_deps_root": "src/third_party/chromium_deps",
  "instaweb_trunk":
    "http://instaweb.googlecode.com/svn/trunk",
  "instaweb_revision": "@4",
}

deps = {
  # Fetch chromium DEPS so we can sync our other dependencies relative
  # to it.
  Var("chromium_deps_root"):
    File(Var("chromium_trunk") + "/src/DEPS" + Var("chromium_revision")),

  "src/site_scons/site_tools":
    (Var("chromium_trunk") + "/src/site_scons/site_tools" +
     Var("chromium_revision")),

  "src/testing":
    Var("chromium_trunk") + "/src/testing" + Var("chromium_revision"),

  "src/third_party/chromium/src/build":
    Var("chromium_trunk") + "/src/build" + Var("chromium_revision"),

  "src/third_party/chromium/src/base":
    Var("chromium_trunk") + "/src/base" + Var("chromium_revision"),

  "src/third_party/chromium/src/net/base":
    Var("chromium_trunk") + "/src/net/base" + Var("chromium_revision"),

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

  "src/third_party/instaweb/src":
    Var("instaweb_trunk") + "/src" + Var("instaweb_revision"),

  "src/third_party/libjpeg":
    (Var("chromium_trunk") + "/src/third_party/libjpeg" +
     Var("chromium_revision")),

  "src/third_party/libpng":
    (Var("chromium_trunk") + "/src/third_party/libpng" +
     Var("chromium_revision")),

  "src/third_party/zlib":
    Var("chromium_trunk") + "/src/third_party/zlib" + Var("chromium_revision"),

  "src/third_party/protobuf2":
    (Var("chromium_trunk") + "/src/third_party/protobuf2" +
     Var("chromium_revision")),

  "src/third_party/scons":
    Var("chromium_trunk") + "/src/third_party/scons" +
      Var("chromium_revision"),

  "src/third_party/modp_b64":
    Var("chromium_trunk") + "/src/third_party/modp_b64" +
      Var("chromium_revision"),

  "src/tools/data_pack":
    Var("chromium_trunk") + "/src/tools/data_pack" + Var("chromium_revision"),

  "src/tools/grit":
    Var("chromium_trunk") + "/src/tools/grit" + Var("chromium_revision"),

  "src/googleurl": From(Var("chromium_deps_root")),
  "src/testing/gtest": From(Var("chromium_deps_root")),
  "src/third_party/protobuf2/src": From(Var("chromium_deps_root")),
  "src/tools/gyp": From(Var("chromium_deps_root")),
}


deps_os = {
  "win": {
    "src/third_party/cygwin": From(Var("chromium_deps_root")),
    "src/third_party/python_24": From(Var("chromium_deps_root")),
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
