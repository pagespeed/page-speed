// Copyright 2013 Google Inc.
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

#if defined(__GNUC__)
#include <cpuid.h>
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

#include "base/logging.h"
#include "pagespeed/core/cpu_compatibility.h"

#if defined(__SSE2__) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 2))
#define COMPILED_WITH_SSE2_ENABLED
#endif

// The cpuid assembly below is only supported on ia32 (and x86-64).
#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
// However native client is unlikely to provide access to this
// information.
#if !defined(__native_client__)
#define IA32_CPUID_SUPPORTED
#endif  // #if !defined(__native_client__)
#endif  // #if defined(_M_X64) || ... || defined(__i386__)

namespace {

#if defined(IA32_CPUID_SUPPORTED)

// cpuid can be invoked in various ways based on the info argument. We
// currently only need the processor info and feature bits, so that's
// the only constant we define for now.
const unsigned kCpuIdProcessorInfoAndFeatureBits = 1;

void cpuid(
    unsigned info, unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx) {
#if defined(__GNUC__)
  // Use gcc's built-in __get_cpuid.
  if (__get_cpuid(info, eax, ebx, ecx, edx) != 1) {
    LOG(ERROR) << "Invalid __get_cpuid level: " << info;
  }
#elif defined(_MSC_VER)
  // Use msvc's build-in __cpuid.
  int cpu_info[4] = {-1};
  __cpuid(cpu_info, info);
  *eax = cpu_info[0];
  *ebx = cpu_info[1];
  *ecx = cpu_info[2];
  *edx = cpu_info[3];
#else
#error "No cpuid implementation available for the current compiler."
#endif
}

bool ProcessorIsSse2Capable() {
  unsigned eax, ebx, ecx, edx;
  cpuid(kCpuIdProcessorInfoAndFeatureBits, &eax, &ebx, &ecx, &edx);
  // 26th bit of edx indicates whether the processor supports sse2
  // instructions. See http://en.wikipedia.org/wiki/CPUID for more
  // details.
  return ((edx & (1 << 26)) != 0);
}

#endif  // #if defined(IA32_CPUID_SUPPORTED)

}  // namespace

namespace pagespeed {

bool IsCpuCompatible() {
#if defined(IA32_CPUID_SUPPORTED)

#if defined(COMPILED_WITH_SSE2_ENABLED)
  if (!ProcessorIsSse2Capable()) {
    LOG(INFO) << "CPU does not support sse2, but binary expects sse2 support.";
    return false;
  }
#endif  // #if defined(COMPILED_WITH_SSE2_ENABLED)

#endif  // #if defined(IA32_CPUID_SUPPORTED)

  return true;
}

}  // namespace pagespeed
