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
#include <iostream>  // for std::cin and std::cout
#include <iterator>
#include <string>

#include "pagespeed/image_compression/gif_reader.h"
#include "pagespeed/image_compression/image_converter.h"
#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/png_optimizer.h"
#include "third_party/gflags/src/google/gflags.h"

DEFINE_string(input_file, "", "Path to input file. '-' to read from stdin.");
DEFINE_string(output_file, "", "Path to output file.");
DEFINE_bool(lossy, false,
            "If true, lossy compression will be performed (assuming the "
            "output format supports lossy compression).");
DEFINE_int32(quality, 85, "Image quality (0-100).");
DEFINE_bool(jpeg_progressive, false,
            "If true, will create a progressive JPEG.");
DEFINE_int32(jpeg_num_scans, -1,
             "Number of progressive scans. "
             "Only applies if --lossy and --jpeg_progressive are set.");
DEFINE_string(jpeg_color_sampling,
              "RETAIN", "Color sampling to use. "
              "Only applies if --lossy is set."
              "Valid values are RETAIN, YUV420, YUV422, YUV444.");
DEFINE_string(input_format, "", "Format of input image. "
              "If unspecified, format will be inferred from file extension. "
              "Valid values are JPEG, GIF, PNG.");
DEFINE_bool(choose_smallest_output_format, false,
            "Chooses the smallest image format for the given input. "
            "Otherwise output format is chosen based on output file "
            "extension.");

using pagespeed::image_compression::ColorSampling;
using pagespeed::image_compression::GifReader;
using pagespeed::image_compression::ImageConverter;
using pagespeed::image_compression::OptimizeJpeg;
using pagespeed::image_compression::OptimizeJpegWithOptions;
using pagespeed::image_compression::PngOptimizer;
using pagespeed::image_compression::PngReader;
using pagespeed::image_compression::PngReaderInterface;

namespace {

enum ImageType {
  UNKNOWN,
  JPEG,
  PNG,
  GIF,
  WEBP,
};

const char* kImageTypeNames[] = {
  "UNKNOWN",
  "JPEG",
  "PNG",
  "GIF",
  "WEBP",
};

ImageType GetOptimizeImageTypeForImageConverterImageType(
    ImageConverter::ImageType t) {
  switch (t) {
    case ImageConverter::IMAGE_PNG: return PNG;
    case ImageConverter::IMAGE_JPEG: return JPEG;
    case ImageConverter::IMAGE_WEBP: return WEBP;
    default: return UNKNOWN;
  }
}

const char* GetImageTypeName(ImageType type) {
  // ptrdiff_t is probably a more appropriate type to cast to but I am
  // not sure of its portability.
  return kImageTypeNames[static_cast<int>(type)];
}

pagespeed::image_compression::ColorSampling GetJpegColorSampling() {
  if (FLAGS_jpeg_color_sampling == "RETAIN") {
    return pagespeed::image_compression::RETAIN;
  } else if (FLAGS_jpeg_color_sampling == "YUV420") {
    return pagespeed::image_compression::YUV420;
  } else if (FLAGS_jpeg_color_sampling == "YUV422") {
    return pagespeed::image_compression::YUV422;
  } else if (FLAGS_jpeg_color_sampling == "YUV444") {
    return pagespeed::image_compression::YUV444;
  } else {
    LOG(DFATAL) << "Unrecognized color sampling. Using default.";
    return pagespeed::image_compression::RETAIN;
  }
}

pagespeed::image_compression::JpegCompressionOptions
    GetJpegCompressionOptions() {
  pagespeed::image_compression::JpegCompressionOptions options;
  options.lossy = FLAGS_lossy;
  options.lossy_options.quality = FLAGS_quality;
  options.lossy_options.color_sampling = GetJpegColorSampling();
  options.lossy_options.num_scans = FLAGS_jpeg_num_scans;
  options.progressive = FLAGS_jpeg_progressive;
  return options;
}

pagespeed::image_compression::WebpConfiguration GetWebpConfiguration() {
  pagespeed::image_compression::WebpConfiguration options;
  options.lossless = FLAGS_lossy ? 0 : 1;
  options.quality = FLAGS_quality;
  return options;
}

// use file extension to determine what optimizer should be used.
ImageType DetermineImageType(const std::string& format,
                             const std::string& filename) {
  if (!format.empty()) {
    if (format == "JPEG") return JPEG;
    if (format == "GIF") return GIF;
    if (format == "PNG") return PNG;
  } else {
    // TODO(bmcquade): consider using a library function to infer the
    // file type from the actual image contents (e.g. magic bytes).
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
      } else if (extension == "webp") {
        return WEBP;
      }
    }
  }
  return UNKNOWN;
}

bool ReadFileToString(const std::string& path, std::string *dest) {
  std::ifstream file_stream;
  file_stream.open(path.c_str(), std::ifstream::in | std::ifstream::binary);
  if (!file_stream.is_open()) {
    return false;
  }
  dest->assign(std::istreambuf_iterator<char>(file_stream),
               std::istreambuf_iterator<char>());
  file_stream.close();
  return (dest->size() > 0);
}

bool HasScanlineReader(ImageType type) {
  return type == PNG || type == GIF;
}

bool HasScanlineWriter(ImageType type) {
  return type == JPEG || type == WEBP;
}

// We currently support the following file format pairs:
// * automatic selection of the output format (output_type == UNKNOWN)
// * any format to itself (except for webp or gif)
// * ScanlineReaderInterface: PNG, GIF
// * ScanlineWriterInterface: JPEG, WEBP
// * GIF->PNG (via legacy custom conversion path)
bool IsValidConversion(ImageType input_type, ImageType output_type) {
  if (input_type == WEBP) {
    // Not currently supported.
    return false;
  }
  if (output_type == GIF) {
    // Not currently supported.
    return false;
  }
  if (output_type == UNKNOWN || input_type == output_type) {
    return true;
  }
  if (HasScanlineReader(input_type) && HasScanlineWriter(output_type)) {
    return true;
  }
  if (input_type == GIF && output_type == PNG) {
    return true;
  }

  return false;
}

bool OptimizeImage(const std::string file_contents,
                   std::string* out_compressed) {
  ImageType input_type =
      DetermineImageType(FLAGS_input_format, FLAGS_input_file);
  if (input_type == UNKNOWN) {
    LOG(ERROR) << "Unable to determine input image type.";
    return false;
  }

  ImageType output_type = UNKNOWN;
  if (!FLAGS_choose_smallest_output_format) {
    output_type = DetermineImageType("", FLAGS_output_file);
    if (output_type == UNKNOWN) {
      output_type = input_type;
      LOG(INFO) << "Unable to determine output image type. Using input type.";
    }
  }

  // If we haven't chosen an output type yet, see if the output type
  // can be inferred from the input type (i.e. see if there is only
  // one possible output type for the given input type).
  if (output_type == UNKNOWN && !HasScanlineReader(input_type)) {
    switch (input_type) {
      case GIF:
        output_type = PNG;
        break;
      default:
        output_type = input_type;
        break;
    }
  }

  if (!IsValidConversion(input_type, output_type)) {
    LOG(ERROR) << "Unable to convert from input_type "
               << GetImageTypeName(input_type)
               << " to output_type "
               << GetImageTypeName(output_type);
    return false;
  }

  // Initialize a bunch of structures that we may need at various
  // points on the code below.
  pagespeed::image_compression::JpegCompressionOptions jpeg_options =
      GetJpegCompressionOptions();
  pagespeed::image_compression::WebpConfiguration webp_config =
      GetWebpConfiguration();
  GifReader gif_reader;
  PngReader png_reader;
  PngReaderInterface* png_reader_interface = NULL;
  switch (input_type) {
    case PNG:
      png_reader_interface = &png_reader;
      break;
    case GIF:
      png_reader_interface = &gif_reader;
      break;
    default:
      png_reader_interface = NULL;
      break;
  }

  bool success = false;
  if (output_type == UNKNOWN) {
    // Convert to all valid output types, and choose the smallest
    // resulting image.
    if (png_reader_interface == NULL) {
      LOG(ERROR) << "Unable to convert input_type " << input_type;
      return false;
    }
    ImageConverter::ImageType out_type =
        ImageConverter::GetSmallestOfPngJpegWebp(
            *png_reader_interface,
            file_contents,
            FLAGS_lossy ? &jpeg_options : NULL,
            &webp_config,
            out_compressed);
    success = (out_type != ImageConverter::IMAGE_NONE);
    // Record the actual type we generated so we can note it on stdout
    // later.
    output_type = GetOptimizeImageTypeForImageConverterImageType(out_type);
  } else if (output_type == JPEG && input_type == JPEG) {
    // Plain old JPEG optimization.
    success = OptimizeJpegWithOptions(
        file_contents, out_compressed, jpeg_options);
  } else if (output_type == PNG) {
    // We need a PngReaderInterface to emit a PNG image.
    if (png_reader_interface == NULL) {
      LOG(ERROR) << "Unable to convert input_type " << input_type;
      return false;
    }
    success = PngOptimizer::OptimizePngBestCompression(
        *png_reader_interface, file_contents, out_compressed);
  } else if (HasScanlineReader(input_type) && HasScanlineWriter(output_type)) {
    if (png_reader_interface == NULL) {
      LOG(ERROR) << "Unable to convert from input_type "
                 << GetImageTypeName(input_type)
                 << " to output_type "
                 << GetImageTypeName(output_type);
      return false;
    }
    if (output_type == WEBP) {
      bool is_opaque = false;
      success = ImageConverter::ConvertPngToWebp(
          *png_reader_interface, file_contents, webp_config, out_compressed,
          &is_opaque);
    } else if (output_type == JPEG) {
      success = ImageConverter::ConvertPngToJpeg(
          *png_reader_interface, file_contents, jpeg_options, out_compressed);
    }
  } else {
    LOG(DFATAL) << "Unexpected input_type,output_type: "
                << input_type << "," << output_type;
    return false;
  }

  if (!success) {
    LOG(ERROR) << "Image compression failed when processing "
               << FLAGS_input_file;
    return false;
  }

  if (input_type != output_type) {
    fprintf(stdout, "Successfully converted to %s.\n",
            GetImageTypeName(output_type));
  } else if (out_compressed->size() >= file_contents.size()) {
    // We were unable to further compress, so output the original image.
    *out_compressed = file_contents;
  }

  return true;
}

void PrintUsage() {
  ::google::ShowUsageWithFlagsRestrict(::google::GetArgv0(), __FILE__);
}

}  // namespace

int main(int argc, char** argv) {
  ::google::SetUsageMessage("Optimize a PNG, JPEG, or GIF image.");
  ::google::ParseCommandLineNonHelpFlags(&argc, &argv, true);

  std::string file_contents;
  if (FLAGS_input_file == "-") {
    // Special case: if user specifies input file as '-', read the
    // input from stdin.
    file_contents.assign(std::istreambuf_iterator<char>(std::cin),
                         std::istreambuf_iterator<char>());
  } else if (!ReadFileToString(FLAGS_input_file, &file_contents)) {
    fprintf(stderr, "Failed to read input file %s.\n",
            FLAGS_input_file.c_str());
    PrintUsage();
    return EXIT_FAILURE;
  }

  std::string compressed;
  bool result = OptimizeImage(file_contents, &compressed);
  if (!result) {
    PrintUsage();
    return EXIT_FAILURE;
  }

  if (compressed.size() >= file_contents.size()) {
    printf("Unable to further optimize image %s.\n", FLAGS_input_file.c_str());
  } else {
    size_t savings = file_contents.size() - compressed.size();
    double percent_savings = 100.0 * savings / file_contents.size();
    printf("Reduced size of %s by %d bytes (%.1f%%).\n",
           FLAGS_input_file.c_str(),
           static_cast<int>(savings),
           percent_savings);
  }
  std::ofstream out(FLAGS_output_file.c_str(),
                    std::ios::out | std::ios::binary);
  if (!out) {
    fprintf(stderr, "Error opening %s for write.\n",
            FLAGS_output_file.c_str());
    PrintUsage();
    return EXIT_FAILURE;
  }
  out.write(compressed.c_str(), compressed.size());
  out.close();
  return EXIT_SUCCESS;
}
