// Copyright 2009 Google Inc. All Rights Reserved.
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

// Command line utility to optimize images

#include <stdio.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>

#include "base/string_number_conversions.h"
#include "pagespeed/image_compression/gif_reader.h"
#include "pagespeed/image_compression/image_converter.h"
#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/png_optimizer.h"

using pagespeed::image_compression::ColorSampling;
using pagespeed::image_compression::GifReader;
using pagespeed::image_compression::ImageConverter;
using pagespeed::image_compression::OptimizeJpeg;
using pagespeed::image_compression::OptimizeJpegWithOptions;
using pagespeed::image_compression::PngOptimizer;
using pagespeed::image_compression::PngReader;

namespace {

enum ImageType {
  NOT_SUPPORTED,
  JPEG,
  PNG,
  GIF,
};

const char *kUsage = "Usage: optimize_image <input> <output> [quality] "
    "[progressive] [num_scans] [color_sampling] \n"
    "quality and progressive are optional, and apply only to lossy formats "
    "(e.g. JPEG). \n"
    "If quality is specified, it should be in the range 1-100. "
    "If unspecified, lossless compression will be performed. \n"
    "If progressive is specified, it should be either 0 or 1. "
    "If unspecified, progressive jpeg is not applied. \n"
    "If num_scans is specified with progressive, we will only output those. \n"
    "If color_sampling is specified, should 0, 1, 2 or 3. "
    "If unspecified, YUV420 is used. only applicable for lossy jpegs. \n";

// use file extension to determine what optimizer should be used.
ImageType DetermineImageType(const std::string& filename) {
  size_t dot_pos = filename.rfind('.');
  if (dot_pos != std::string::npos) {
    std::string extension;
    std::transform(filename.begin() + dot_pos + 1, filename.end(),
                   std::back_insert_iterator<std::string>(extension),
                   tolower);
    if (extension == "jpg" || extension == "jpeg") {
      return JPEG;
    } else if (extension == "png") {
      return PNG;
    } else if (extension == "gif") {
      return GIF;
    }
  }
  return NOT_SUPPORTED;
}

bool OptimizeImage(const char* infile, const char* outfile, int opt_quality,
                   bool progressive, int num_scans, ColorSampling color_sampling) {
  std::string filename(infile);
  std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
  if (!in) {
    fprintf(stderr, "Could not read input from %s\n", filename.c_str());
    return false;
  }

  in.seekg (0, std::ios::end);
  int length = in.tellg();
  in.seekg (0, std::ios::beg);

  char* buffer = new char[length];
  in.read(buffer, length);
  in.close();

  std::string file_contents(buffer, length);
  delete[] buffer;

  std::string compressed;
  ImageType type = DetermineImageType(filename);

  bool success = false;
  if (type == JPEG) {
    pagespeed::image_compression::JpegCompressionOptions options;
    if (opt_quality > 0) {
      options.lossy = true;
      options.lossy_options.quality = opt_quality;
      options.lossy_options.color_sampling = color_sampling;
      options.lossy_options.num_scans = num_scans;
    }
    options.progressive = progressive;
    success = OptimizeJpegWithOptions(file_contents, &compressed, options);
  } else if (type == PNG) {
    PngReader reader;
    if (opt_quality > 0) {
      pagespeed::image_compression::JpegCompressionOptions options;
      options.lossy = true;
      options.lossy_options.quality = opt_quality;
      options.lossy_options.num_scans = num_scans;
      bool is_png;
      success = ImageConverter::OptimizePngOrConvertToJpeg(
          reader, file_contents, options, &compressed, &is_png);
    } else  {
      success = PngOptimizer::OptimizePngBestCompression(
          reader, file_contents, &compressed);
    }
  } else if (type == GIF) {
    GifReader reader;
    success = PngOptimizer::OptimizePngBestCompression(reader,
                                                       file_contents,
                                                       &compressed);
  } else {
    fprintf(stderr,
            "Unsupported image type when processing %s\n",
            filename.c_str());
    return false;
  }

  if (!success) {
    fprintf(stderr,
            "Image compression failed when processing %s\n",
            filename.c_str());
    return false;
  }

  if (compressed.size() >= file_contents.size()) {
    compressed = file_contents;
  }

  if (outfile != NULL) {
    std::ofstream out(outfile, std::ios::out | std::ios::binary);
    if (!out) {
      fprintf(stderr, "Error opening %s for write\n", outfile);
      return false;
    }
    out.write(compressed.c_str(), compressed.size());
    out.close();
  }

  return true;
}

}  // namespace



int main(int argc, char** argv) {
  if (argc == 1) {
    fprintf(stderr, "%s", kUsage);
    return EXIT_SUCCESS;
  }

  // If running in batch mode, optimize every image specified on the
  // command line. Do not write any optimized files to disk. This mode
  // can be used to determine how long it takes to optimize a set of
  // files.
  if (strcmp("--batch", argv[1]) == 0) {
    for (int i = 2; i < argc; ++i) {
      OptimizeImage(argv[i], NULL, 0, false, -1, ColorSampling::YUV420);
    }
    return EXIT_SUCCESS;
  }

  // Otherwise we are running in normal mode, where the arguments are
  // <infile> <outfile>.
  if (argc < 3 && argc > 7) {
    fprintf(stderr, "%s", kUsage);
    return EXIT_FAILURE;
  }

  int quality = 0;
  if (argc >= 4) {
    if (!base::StringToInt(argv[3], &quality) ||
        quality < 0 || quality > 100) {
      fprintf(stderr, "%s", kUsage);
      return EXIT_FAILURE;
    }
  }

  bool progressive = false;
  if (argc >= 5) {
    int progressive_int = -1;
    if (!base::StringToInt(argv[4], &progressive_int) ||
        (progressive_int != 0 && progressive_int != 1)) {
      fprintf(stderr, "%s", kUsage);
      return EXIT_FAILURE;
    } else if (progressive_int == 1) {
      progressive = true;
    }
  }

  int num_scans = -1;
  if (argc >= 6) {
    int num_scans_int = -1;
    if (!base::StringToInt(argv[5], &num_scans_int) ||
        num_scans_int < 0) {
      fprintf(stderr, "%s", kUsage);
      return EXIT_FAILURE;
    } else {
      num_scans = num_scans_int;
    }
  }

  ColorSampling color_sampling = ColorSampling::YUV420;
  if (argc == 7) {
    int color_sampling_int = -1;
     if (!base::StringToInt(argv[6], &color_sampling_int) ||
         color_sampling_int < 0 || color_sampling_int > 3) {
       fprintf(stderr, "%s", kUsage);
       return EXIT_FAILURE;
     } else {
       color_sampling = static_cast<ColorSampling>(color_sampling_int);
     }
  }

  return OptimizeImage(argv[1], argv[2], quality, progressive, num_scans,
                       color_sampling) ? EXIT_SUCCESS : EXIT_FAILURE;
}
