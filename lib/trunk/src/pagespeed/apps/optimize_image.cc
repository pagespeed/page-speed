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

#include "pagespeed/image_compression/gif_reader.h"
#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/png_optimizer.h"

using pagespeed::image_compression::GifReader;
using pagespeed::image_compression::OptimizeJpeg;
using pagespeed::image_compression::PngOptimizer;
using pagespeed::image_compression::PngReader;

namespace {

enum ImageType {
  NOT_SUPPORTED,
  JPEG,
  PNG,
  GIF,
};

const char *kUsage = "Usage: optimize_image <input> <output>\n";

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

bool OptimizeImage(const char* infile, const char* outfile) {
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
    success = OptimizeJpeg(file_contents, &compressed);
  } else if (type == PNG) {
    PngReader reader;
    success = PngOptimizer::OptimizePng(reader,
                                        file_contents,
                                        &compressed);
  } else if (type == GIF) {
    GifReader reader;
    success = PngOptimizer::OptimizePng(reader,
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
      OptimizeImage(argv[i], NULL);
    }
    return EXIT_SUCCESS;
  }

  // Otherwise we are running in normal mode, where the arguments are
  // <infile> <outfile>.
  if (argc != 3) {
    fprintf(stderr, "%s", kUsage);
    return EXIT_FAILURE;
  }

  return OptimizeImage(argv[1], argv[2]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
