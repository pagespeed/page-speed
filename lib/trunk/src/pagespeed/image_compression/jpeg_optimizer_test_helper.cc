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

#include "pagespeed/image_compression/jpeg_optimizer_test_helper.h"
#include "pagespeed/image_compression/jpeg_reader.h"

#include <setjmp.h>

extern "C" {
#ifdef USE_SYSTEM_LIBJPEG
#include "jpeglib.h"
#include "jerror.h"
#else
#include "third_party/libjpeg/jpeglib.h"
#include "third_party/libjpeg/jerror.h"
#endif
}

namespace pagespeed_testing {
namespace image_compression {

bool GetJpegNumComponentsAndSamplingFactors(
    const std::string& jpeg,
    int* out_num_components,
    int* out_h_samp_factor,
    int* out_v_samp_factor) {
  pagespeed::image_compression::JpegReader reader;
  jpeg_decompress_struct* jpeg_decompress = reader.decompress_struct();

  jmp_buf env;
  if (setjmp(env)) {
    return false;
  }

  // Need to install env so that it will be longjmp()ed to on error.
  jpeg_decompress->client_data = static_cast<void *>(&env);

  reader.PrepareForRead(jpeg);
  jpeg_read_header(jpeg_decompress, TRUE);
  *out_num_components = jpeg_decompress->num_components;
  *out_h_samp_factor = jpeg_decompress->comp_info[0].h_samp_factor;
  *out_v_samp_factor = jpeg_decompress->comp_info[0].v_samp_factor;
  return true;
}

}  // namespace image_compression
}  // namespace pagespeed_testing
