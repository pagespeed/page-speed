// Copyright 2011 Google Inc.
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
//
// This header only exists to avoid including jpeglib.h, etc directly
// in jpeg_optimizer_test.cc, since doing so causes symbol collisions on
// Windows.

#include <string>

namespace pagespeed_testing {
namespace image_compression {

// Helper that extracts the number of components and h/v sampling factors.
bool GetJpegNumComponentsAndSamplingFactors(
    const std::string& jpeg,
    int* out_num_components,
    int* out_h_samp_factor,
    int* out_v_samp_factor);

}  // namespace image_compression
}  // namespace pagespeed_testing
