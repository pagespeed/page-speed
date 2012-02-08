#!/usr/bin/python
#
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
#
# Wrapper script that configures our build environment to build for
# Native Client.

import os
import subprocess
import sys

script_dir = os.path.dirname(__file__)
default_nacl_home = os.path.normpath(
    os.path.join(script_dir, 'third_party', 'nacl_sdk'))
default_nacl_bundle = 'pepper_17'

nacl_home = os.environ.get('NACL_HOME', default_nacl_home)
nacl_bundle = os.environ.get('NACL_BUNDLE', default_nacl_bundle)

if not os.path.isdir(nacl_home):
  print 'Unable to find NACL_HOME at %s.' % nacl_home
  sys.exit(1)

naclsdk_bin = os.path.normpath(os.path.join(nacl_home, 'naclsdk'))
subprocess.call([naclsdk_bin, 'update', nacl_bundle])

host_arch, host_arch_err = subprocess.Popen(
    "uname -m", shell=True, stdout=subprocess.PIPE).communicate()
if host_arch_err:
  print host_arch_err
  sys.exit(1)

host_arch = host_arch.strip()
if host_arch == "x86_64":
  nacl_bin_prefix = "x86_64-nacl"
else:
  nacl_bin_prefix = "i686-nacl"

nacl_bin_dir = os.path.normpath(os.path.join(
        nacl_home, nacl_bundle, 'toolchain', 'linux_x86_newlib', 'bin'))

gyp_defines = os.environ.get('GYP_DEFINES', '')
gyp_defines = 'build_nacl=1' + ' ' + gyp_defines
os.putenv('GYP_DEFINES', gyp_defines)

os.putenv('CC.target', os.path.join(nacl_bin_dir, nacl_bin_prefix) + '-gcc')
os.putenv('CXX.target', os.path.join(nacl_bin_dir, nacl_bin_prefix) + '-g++')
os.putenv('LINK.target', os.path.join(nacl_bin_dir, nacl_bin_prefix) + '-g++')
os.putenv('AR.target', os.path.join(nacl_bin_dir, nacl_bin_prefix) + '-ar')
os.putenv('RANLIB.target',
          os.path.join(nacl_bin_dir, nacl_bin_prefix) + '-ranlib')

subprocess.call("gclient runhooks", shell=True)

make_cmd = "make " + ' '.join(sys.argv[1:])
subprocess.call(make_cmd, shell=True)
