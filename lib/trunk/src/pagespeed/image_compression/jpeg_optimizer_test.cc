/**
 * Copyright 2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: Bryan McQuade, Matthew Steele

#include <fstream>
#include <string>
#include <vector>

#include "base/basictypes.h"

extern "C" {
#ifdef USE_SYSTEM_LIBJPEG
#include "jpeglib.h"
#include "jerror.h"
#else
#include "third_party/libjpeg/jpeglib.h"
#include "third_party/libjpeg/jerror.h"
#endif
}

#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/jpeg_reader.h"
#include "pagespeed/image_compression/jpeg_optimizer_test_helper.h"
#include "pagespeed/testing/pagespeed_test.h"

// DO NOT INCLUDE LIBJPEG HEADERS HERE. Doing so causes build errors
// on Windows. If you need to call out to libjpeg, please add helper
// methods in jpeg_optimizer_test_helper.h.

namespace {

using pagespeed::image_compression::ColorSampling;
using pagespeed::image_compression::JpegCompressionOptions;
using pagespeed::image_compression::JpegLossyOptions;
using pagespeed::image_compression::JpegReader;
using pagespeed::image_compression::OptimizeJpeg;
using pagespeed::image_compression::OptimizeJpegWithOptions;
using pagespeed_testing::image_compression::GetJpegNumComponentsAndSamplingFactors;

// The JPEG_TEST_DIR_PATH macro is set by the gyp target that builds this file.
const std::string kJpegTestDir = IMAGE_TEST_DIR_PATH "jpeg/";
// Marker for APPN segment can obtained by adding N to JPEG_APP0. There is no
// direct constant to refer them. The offsets here are part of jpeg codex, for
// example JPEG_APP0 + 2 refers to APP2 which should always correspond to color
// profile information.
const int kColorProfileMarker = JPEG_APP0 + 2;
const int kExifDataMarker = JPEG_APP0 + 1;
const char* kAppSegmentsJpegFile = "app_segments.jpg";

struct ImageCompressionInfo {
  const char* filename;
  size_t original_size;
  size_t compressed_size;
  size_t lossy_compressed_size;
  size_t progressive_size;
  size_t progressive_and_lossy_compressed_size;
};

ImageCompressionInfo kValidImages[] = {
  { "sjpeg1.jpg", 1552, 1536, 1165, 1774, 1410 },
  { "sjpeg2.jpg", 3612, 3283, 3630, 3475, 3798 },
  { "sjpeg3.jpg", 44084, 41664, 26924, 40997, 25814 },
  { "sjpeg4.jpg", 168895, 168240, 51389, 162867, 49186 },
  { "sjpeg6.jpg", 149600, 147163, 89671, 146038, 84641 },
  { "test411.jpg", 6883, 4367, 3709, 4540, 3854 },
  { "test420.jpg", 6173, 3657, 3653, 3796, 3793 },
  { "test422.jpg", 6501, 3985, 3712, 4152, 3849 },
  { "testgray.jpg", 5014, 3072, 3060, 3094, 3078 },
};

const char *kInvalidFiles[] = {
  "notajpeg.png",  // A png.
  "notajpeg.gif",  // A gif.
  "emptyfile.jpg", // A zero-byte file.
  "corrupt.jpg",   // Invalid huffman code in the image data section.
};

// Given one of the above file names, read the contents of the file into the
// given destination string.

void ReadJpegToString(const std::string &file_name, std::string *dest) {
  const std::string path = kJpegTestDir + file_name;
  pagespeed_testing::ReadFileToString(path, dest);
}

void WriteStringToFile(const std::string &file_name, std::string &src) {
  const std::string path = kJpegTestDir + file_name;
  std::ofstream stream;
  stream.open(path.c_str(), std::ofstream::out | std::ofstream::binary);
  stream.write(src.c_str(), src.size());
  stream.close();
}

const size_t kValidImageCount = arraysize(kValidImages);
const size_t kInvalidFileCount = arraysize(kInvalidFiles);

void AssertColorSampling(const std::string& data,
                         int expected_h_sampling_factor,
                         int expected_v_sampling_factor) {
  int num_components, h_sampling_factor, v_sampling_factor;
  ASSERT_TRUE(GetJpegNumComponentsAndSamplingFactors(data,
                                                     &num_components,
                                                     &h_sampling_factor,
                                                     &v_sampling_factor));
  ASSERT_LE(1, num_components);
  ASSERT_EQ(expected_h_sampling_factor, h_sampling_factor);
  ASSERT_EQ(expected_v_sampling_factor, v_sampling_factor);
}

bool IsJpegSegmentPresent(const std::string& data, int segment) {
  JpegReader reader;
  jpeg_decompress_struct* jpeg_decompress = reader.decompress_struct();

  jmp_buf env;
  if (setjmp(env)) {
    return false;
  }

  // Need to install env so that it will be longjmp()ed to on error.
  jpeg_decompress->client_data = static_cast<void *>(&env);

  reader.PrepareForRead(data);
  jpeg_save_markers(jpeg_decompress, segment, 0xFFFF);
  jpeg_read_header(jpeg_decompress, TRUE);

  bool is_marker_present = false;
  for (jpeg_saved_marker_ptr marker = jpeg_decompress->marker_list;
       marker != NULL; marker = marker->next) {
    if (marker->marker == segment) {
      is_marker_present = true;
      break;
    }
  }

  return is_marker_present;
}

int GetNumScansInJpeg(const std::string& data) {
  JpegReader reader;
  jpeg_decompress_struct* jpeg_decompress = reader.decompress_struct();

  jmp_buf env;
  if (setjmp(env)) {
    return false;
  }

  // Need to install env so that it will be longjmp()ed to on error.
  jpeg_decompress->client_data = static_cast<void *>(&env);

  reader.PrepareForRead(data);
  jpeg_read_header(jpeg_decompress, TRUE);

  jpeg_decompress->buffered_image = true;
  jpeg_start_decompress(jpeg_decompress);

  int num_scans = 0;
  while (!jpeg_input_complete(jpeg_decompress)) {
   if (jpeg_consume_input(jpeg_decompress) == JPEG_SCAN_COMPLETED) {
     num_scans++;
   }
  }

  return num_scans;
}

void AssertJpegOptimizeWithSampling(
    const std::string &src_data, std::string* dest_data,
    ColorSampling color_sampling, int h_sampling_factor,
    int v_sampling_factor) {
  dest_data->clear();
  JpegCompressionOptions options;
  options.lossy = true;
  options.lossy_options.color_sampling = color_sampling;

  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, dest_data, options));
  AssertColorSampling(*dest_data, h_sampling_factor, v_sampling_factor);
}

TEST(JpegOptimizerTest, ValidJpegs) {
  for (size_t i = 0; i < kValidImageCount; ++i) {
    std::string src_data;
    ReadJpegToString(kValidImages[i].filename, &src_data);
    std::string dest_data;
    ASSERT_TRUE(OptimizeJpeg(src_data, &dest_data));
    EXPECT_EQ(kValidImages[i].original_size, src_data.size())
        << kValidImages[i].filename;
    EXPECT_EQ(kValidImages[i].compressed_size, dest_data.size())
        << kValidImages[i].filename;

    // Uncomment this next line for debugging:
    //WriteStringToFile(std::string("z") + kValidImages[i].filename, dest_data);

    ASSERT_LE(dest_data.size(), src_data.size());
  }
}

TEST(JpegOptimizerTest, ValidJpegsLossy) {
  for (size_t i = 0; i < kValidImageCount; ++i) {
    std::string src_data;
    ReadJpegToString(kValidImages[i].filename, &src_data);
    pagespeed::image_compression::JpegCompressionOptions options;
    options.lossy = true;
    std::string dest_data;
    ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options))
        << kValidImages[i].filename;
    EXPECT_EQ(kValidImages[i].original_size, src_data.size())
        << kValidImages[i].filename;
    EXPECT_EQ(kValidImages[i].lossy_compressed_size, dest_data.size())
        << kValidImages[i].filename;

    // Uncomment this next line for debugging:
    //WriteStringToFile(std::string("l") + kValidImages[i].filename, dest_data);
  }
}

TEST(JpegOptimizerTest, ValidJpegLossyAndColorSampling) {
  int test_422_file_idx = 7;
  std::string src_data;
  std::string src_filename = kValidImages[test_422_file_idx].filename;
  ReadJpegToString(src_filename, &src_data);

  JpegCompressionOptions options;
  options.lossy = true;

  std::string dest_data;
  // Calling optimize will use default color sampling which is 420.
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  size_t lossy_420_size =
      kValidImages[test_422_file_idx].lossy_compressed_size;
  EXPECT_EQ(lossy_420_size, dest_data.size()) << src_filename;
  AssertColorSampling(dest_data, 2, 2);

  // Calling optimize with ColorSampling::YUV420 will give samping as 420.
  AssertJpegOptimizeWithSampling(src_data, &dest_data,
                                 pagespeed::image_compression::YUV420, 2, 2);
  EXPECT_EQ(lossy_420_size, dest_data.size()) << src_filename;

  // Calling optimize with ColorSampling::RETAIN will leave samping as 422.
  AssertJpegOptimizeWithSampling(src_data, &dest_data,
                                 pagespeed::image_compression::RETAIN, 2, 1);
  size_t lossy_retain_size = dest_data.size();
  EXPECT_GT(lossy_retain_size, lossy_420_size) << src_filename;

  // Calling optimize with ColorSampling::YUV422 will give samping as 422.
  AssertJpegOptimizeWithSampling(src_data, &dest_data,
                                 pagespeed::image_compression::YUV422, 2, 1);
  EXPECT_EQ(lossy_retain_size, dest_data.size()) << src_filename;

  // Calling optimize with ColorSampling::YUV444 will give samping as 444.
  AssertJpegOptimizeWithSampling(src_data, &dest_data,
                                 pagespeed::image_compression::YUV444, 1, 1);
  EXPECT_LT(lossy_retain_size, dest_data.size()) << src_filename;
}

TEST(JpegOptimizerTest, ValidJpegRetainColorProfile) {
  std::string src_data;
  ReadJpegToString(kAppSegmentsJpegFile, &src_data);

  std::string dest_data;

  // Testing lossless flow.
  JpegCompressionOptions options;
  options.retain_color_profile = true;

  ASSERT_TRUE(IsJpegSegmentPresent(src_data, kColorProfileMarker));
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  ASSERT_TRUE(IsJpegSegmentPresent(dest_data, kColorProfileMarker));

  options.retain_color_profile = false;
  dest_data.clear();
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  ASSERT_FALSE(IsJpegSegmentPresent(dest_data, kColorProfileMarker));

  // Testing lossy flow.
  options.lossy = true;
  options.retain_color_profile = true;
  dest_data.clear();
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  ASSERT_TRUE(IsJpegSegmentPresent(dest_data, kColorProfileMarker));

  options.retain_color_profile = false;
  dest_data.clear();
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  ASSERT_FALSE(IsJpegSegmentPresent(dest_data, kColorProfileMarker));
}

TEST(JpegOptimizerTest, ValidJpegRetainExifData) {
  std::string src_data;
  ReadJpegToString(kAppSegmentsJpegFile, &src_data);

  std::string dest_data;

  // Testing lossless flow.
  JpegCompressionOptions options;
  options.retain_exif_data = true;

  ASSERT_TRUE(IsJpegSegmentPresent(src_data, kExifDataMarker));
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  ASSERT_TRUE(IsJpegSegmentPresent(dest_data, kExifDataMarker));

  options.retain_exif_data = false;
  dest_data.clear();
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  ASSERT_FALSE(IsJpegSegmentPresent(dest_data, kExifDataMarker));

  // Testing lossy flow.
  options.lossy = true;
  options.retain_exif_data = true;
  dest_data.clear();
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  ASSERT_TRUE(IsJpegSegmentPresent(dest_data, kExifDataMarker));

  options.retain_exif_data = false;
  dest_data.clear();
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  ASSERT_FALSE(IsJpegSegmentPresent(dest_data, kExifDataMarker));
}

TEST(JpegOptimizerTest, ValidJpegLossyWithNProgressiveScans) {
  std::string src_data;
  ReadJpegToString(kAppSegmentsJpegFile, &src_data);

  std::string dest_data;

  // Testing lossless flow.
  JpegCompressionOptions options;
  options.progressive = true;

  EXPECT_EQ(1, GetNumScansInJpeg(src_data));
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  int num_scans = GetNumScansInJpeg(dest_data);
  EXPECT_LT(1, num_scans);

  dest_data.clear();
  options.lossy = true;
  options.lossy_options.num_scans = 3;
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  EXPECT_EQ(3, GetNumScansInJpeg(dest_data));

  dest_data.clear();
  // jpeg has max scan limit based on image color spaces, So trying to set
  // num scans to a large value should be handled gracefully and default to
  // jpeg scan limit if the specified value is greater.
  options.lossy_options.num_scans = 1000;
  ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  EXPECT_EQ(num_scans, GetNumScansInJpeg(dest_data));
}

TEST(JpegOptimizerTest, ValidJpegsProgressive) {
  for (size_t i = 0; i < kValidImageCount; ++i) {
    std::string src_data;
    ReadJpegToString(kValidImages[i].filename, &src_data);
    pagespeed::image_compression::JpegCompressionOptions options;
    options.progressive = true;
    std::string dest_data;
    ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options))
        << kValidImages[i].filename;
    EXPECT_EQ(kValidImages[i].original_size, src_data.size())
        << kValidImages[i].filename;
    EXPECT_EQ(kValidImages[i].progressive_size, dest_data.size())
        << kValidImages[i].filename;

    // Uncomment this next line for debugging:
    //WriteStringToFile(std::string("p") + kValidImages[i].filename, dest_data);
  }
}

TEST(JpegOptimizerTest, ValidJpegsProgressiveAndLossy) {
  for (size_t i = 0; i < kValidImageCount; ++i) {
    std::string src_data;
    ReadJpegToString(kValidImages[i].filename, &src_data);
    pagespeed::image_compression::JpegCompressionOptions options;
    options.lossy = true;
    options.progressive = true;
    std::string dest_data;
    ASSERT_TRUE(OptimizeJpegWithOptions(src_data, &dest_data, options))
        << kValidImages[i].filename;
    EXPECT_EQ(kValidImages[i].original_size, src_data.size())
        << kValidImages[i].filename;
    EXPECT_EQ(kValidImages[i].progressive_and_lossy_compressed_size,
              dest_data.size()) << kValidImages[i].filename;

    // Uncomment this next line for debugging:
    //WriteStringToFile(std::string("b") + kValidImages[i].filename, dest_data);
  }
}

TEST(JpegOptimizerTest, InvalidJpegs) {
  for (size_t i = 0; i < kInvalidFileCount; ++i) {
    std::string src_data;
    ReadJpegToString(kInvalidFiles[i], &src_data);
    std::string dest_data;
    ASSERT_FALSE(OptimizeJpeg(src_data, &dest_data));
  }
}

TEST(JpegOptimizerTest, InvalidJpegsLossy) {
  for (size_t i = 0; i < kInvalidFileCount; ++i) {
    std::string src_data;
    ReadJpegToString(kInvalidFiles[i], &src_data);
    JpegCompressionOptions options;
    options.lossy = true;
    std::string dest_data;
    ASSERT_FALSE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  }
}

TEST(JpegOptimizerTest, InvalidJpegsProgressive) {
  for (size_t i = 0; i < kInvalidFileCount; ++i) {
    std::string src_data;
    ReadJpegToString(kInvalidFiles[i], &src_data);
    JpegCompressionOptions options;
    options.progressive = true;
    std::string dest_data;
    ASSERT_FALSE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  }
}

TEST(JpegOptimizerTest, InvalidJpegsProgressiveAndLossy) {
  for (size_t i = 0; i < kInvalidFileCount; ++i) {
    std::string src_data;
    ReadJpegToString(kInvalidFiles[i], &src_data);
    JpegCompressionOptions options;
    options.lossy = true;
    options.progressive = true;
    std::string dest_data;
    ASSERT_FALSE(OptimizeJpegWithOptions(src_data, &dest_data, options));
  }
}

// Test that after reading an invalid jpeg, the reader cleans its state so that
// it can read a correct jpeg again.
TEST(JpegOptimizerTest, CleanupAfterReadingInvalidJpeg) {
  // Compress each input image with a reinitialized JpegOptimizer.
  // We will compare these files with the output we get from
  // a JpegOptimizer that had an error.
  std::vector<std::string> correctly_compressed;
  for (size_t i = 0; i < kValidImageCount; ++i) {
    std::string src_data;
    ReadJpegToString(kValidImages[i].filename, &src_data);
    correctly_compressed.push_back("");
    std::string &dest_data = correctly_compressed.back();
    ASSERT_TRUE(OptimizeJpeg(src_data, &dest_data));
  }

  // The invalid files are all invalid in different ways, and we want to cover
  // all the ways jpeg decoding can fail.  So, we want at least as many valid
  // images as invalid ones.
  ASSERT_GE(kValidImageCount, kInvalidFileCount);

  for (size_t i = 0; i < kInvalidFileCount; ++i) {
    std::string invalid_src_data;
    ReadJpegToString(kInvalidFiles[i], &invalid_src_data);
    std::string invalid_dest_data;

    std::string valid_src_data;
    ReadJpegToString(kValidImages[i].filename, &valid_src_data);
    std::string valid_dest_data;

    ASSERT_FALSE(OptimizeJpeg(invalid_src_data, &invalid_dest_data));
    ASSERT_TRUE(OptimizeJpeg(valid_src_data, &valid_dest_data));

    // Diff the jpeg created by CreateOptimizedJpeg() with the one created
    // with a reinitialized JpegOptimizer.
    ASSERT_EQ(valid_dest_data, correctly_compressed.at(i));
  }
}

}  // namespace
