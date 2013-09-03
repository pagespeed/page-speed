// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Various C methods not provided by the NaCL runtime that we stub
// here so we can link successfully.

#ifndef __native_client__
#error This file should only be used when compiling for Native Client.
#endif

#include <errno.h>

// Chromium logging.cc wants to call unlink() but obviously NaCL
// doesn't support this (no filesystem access) so we simply stub it
// out and return a failure code.
int unlink(const char *pathname) {
  // EPERM indicates that the operation is not allowed.
  errno = EPERM;
  return -1;
}
