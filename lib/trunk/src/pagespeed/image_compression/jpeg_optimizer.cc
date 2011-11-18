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

#include "pagespeed/image_compression/jpeg_optimizer.h"

#include <setjmp.h>  // for setjmp/longjmp
#include <stdio.h>  // provides FILE for jpeglib (needed for certain builds)
#include <string.h>  // for memset

#include "base/basictypes.h"
#include "base/logging.h"

extern "C" {
#ifdef USE_SYSTEM_LIBJPEG
#include "jpeglib.h"
#include "jerror.h"
#else
#include "third_party/libjpeg/jpeglib.h"
#include "third_party/libjpeg/jerror.h"
#endif
}

#include "pagespeed/image_compression/jpeg_reader.h"

namespace {

// Unfortunately, libjpeg normally only supports writing images to C FILE
// pointers, wheras we want to write to a C++ string.  Fortunately, libjpeg
// also provides an extension mechanism.  Below, we define a new kind of
// jpeg_destination_mgr for writing to strings.

// The below code was adapted from the JPEGMemoryReader class that can be found
// in src/o3d/core/cross/bitmap_jpg.cc in the Chromium source tree (r29423).
// That code is Copyright 2009, Google Inc.

#define DESTINATION_MANAGER_BUFFER_SIZE 4096
struct DestinationManager : public jpeg_destination_mgr {
  JOCTET buffer[DESTINATION_MANAGER_BUFFER_SIZE];
  std::string *str;
};

METHODDEF(void) InitDestination(j_compress_ptr cinfo) {
  DestinationManager &dest =
      *reinterpret_cast<DestinationManager*>(cinfo->dest);

  dest.next_output_byte = dest.buffer;
  dest.free_in_buffer = DESTINATION_MANAGER_BUFFER_SIZE;
};

METHODDEF(boolean) EmptyOutputBuffer(j_compress_ptr cinfo) {
  DestinationManager &dest =
      *reinterpret_cast<DestinationManager*>(cinfo->dest);

  dest.str->append(reinterpret_cast<char*>(dest.buffer),
                   DESTINATION_MANAGER_BUFFER_SIZE);

  dest.free_in_buffer = DESTINATION_MANAGER_BUFFER_SIZE;
  dest.next_output_byte = dest.buffer;

  return TRUE;
};

METHODDEF(void) TermDestination(j_compress_ptr cinfo) {
  DestinationManager &dest =
      *reinterpret_cast<DestinationManager*>(cinfo->dest);

  const size_t datacount =
      DESTINATION_MANAGER_BUFFER_SIZE - dest.free_in_buffer;
  if (datacount > 0) {
    dest.str->append(reinterpret_cast<char*>(dest.buffer), datacount);
  }
};

// Call this function on a j_compress_ptr to install a writer that will write
// to the given string.
void JpegStringWriter(j_compress_ptr cinfo, std::string *data_dest) {
  if (cinfo->dest == NULL) {
    cinfo->dest = (struct jpeg_destination_mgr*)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                  sizeof(DestinationManager));
  }
  DestinationManager &dest =
      *reinterpret_cast<DestinationManager*>(cinfo->dest);

  dest.str = data_dest;

  dest.init_destination = InitDestination;
  dest.empty_output_buffer = EmptyOutputBuffer;
  dest.term_destination = TermDestination;
}

// ErrorExit() is installed as a callback, called on errors
// encountered within libjpeg.  The longjmp jumps back
// to the setjmp in JpegOptimizer::CreateOptimizedJpeg().
void ErrorExit(j_common_ptr jpeg_state_struct) {
  jmp_buf *env = static_cast<jmp_buf *>(jpeg_state_struct->client_data);
  (*jpeg_state_struct->err->output_message)(jpeg_state_struct);
  if (env)
    longjmp(*env, 1);
}

// OutputMessageFromReader is called by libjpeg code on an error when reading.
// Without this function, a default function would print to standard error.
void OutputMessage(j_common_ptr jpeg_decompress) {
  // The following code is handy for debugging.
  /*
  char buf[JMSG_LENGTH_MAX];
  (*jpeg_decompress->err->format_message)(jpeg_decompress, buf);
  LOG(INFO) << "JPEG Reader Error: " << buf;
  */
}

class JpegOptimizer {
 public:
  JpegOptimizer();
  ~JpegOptimizer();

  // Take the given input file and compress it, either losslessly or lossily,
  // depending on the passed in options.  Note that the options parameter can be
  // null, in which case the default options are used.
  // If this function fails (returns false), it can be called again.
  // @return true on success, false on failure.
  bool CreateOptimizedJpeg(
      const std::string &original,
      std::string *compressed,
      const pagespeed::image_compression::JpegCompressionOptions *options);

 private:
  bool DoCreateOptimizedJpeg(
      const std::string &original,
      jpeg_decompress_struct *jpeg_decompress,
      std::string *compressed,
      const pagespeed::image_compression::JpegCompressionOptions *options);

  bool OptimizeLossless(jpeg_decompress_struct *jpeg_decompress,
                        std::string *compressed,
                        bool progressive);

  bool OptimizeLossy(jpeg_decompress_struct *jpeg_decompress,
                     std::string *compressed,
                     int jpeg_quality,
                     bool progressive);

  pagespeed::image_compression::JpegReader reader_;

  // Structures for jpeg compression.
  jpeg_compress_struct jpeg_compress_;
  jpeg_error_mgr compress_error_;

  DISALLOW_COPY_AND_ASSIGN(JpegOptimizer);
};

JpegOptimizer::JpegOptimizer() {
  memset(&jpeg_compress_, 0, sizeof(jpeg_compress_struct));
  memset(&compress_error_, 0, sizeof(jpeg_error_mgr));

  jpeg_compress_.err = jpeg_std_error(&compress_error_);
  compress_error_.error_exit = &ErrorExit;
  compress_error_.output_message = &OutputMessage;
  jpeg_create_compress(&jpeg_compress_);
}

JpegOptimizer::~JpegOptimizer() {
  jpeg_destroy_compress(&jpeg_compress_);
}

bool JpegOptimizer::OptimizeLossy(
    jpeg_decompress_struct *jpeg_decompress,
    std::string *compressed,
    int jpeg_quality,
    bool progressive) {
  // Copy data from the source to the dest.
  jpeg_compress_.image_width = jpeg_decompress->image_width;
  jpeg_compress_.image_height = jpeg_decompress->image_height;
  jpeg_compress_.input_components = jpeg_decompress->num_components;

  // Persist the input file's colorspace.
  jpeg_decompress->out_color_space = jpeg_decompress->jpeg_color_space;
  jpeg_compress_.in_color_space = jpeg_decompress->jpeg_color_space;

  // Set the default options.
  jpeg_set_defaults(&jpeg_compress_);

  // Set optimize huffman to true.
  jpeg_compress_.optimize_coding = TRUE;

  if (jpeg_quality > 0) {
    // Set the compression parameters if set and lossy compression is enabled,
    // else use the defaults.
    jpeg_set_quality(&jpeg_compress_, jpeg_quality, 1);
  }
  if (progressive) {
    jpeg_simple_progression(&jpeg_compress_);
  }

  // Prepare to write to a string.
  JpegStringWriter(&jpeg_compress_, compressed);

  jpeg_start_compress(&jpeg_compress_, TRUE);
  jpeg_start_decompress(jpeg_decompress);

  // Make sure input/output parameters are configured correctly.
  DCHECK(jpeg_compress_.image_width == jpeg_decompress->output_width);
  DCHECK(jpeg_compress_.image_height == jpeg_decompress->output_height);
  DCHECK(jpeg_compress_.input_components == jpeg_decompress->output_components);
  DCHECK(jpeg_compress_.in_color_space == jpeg_decompress->out_color_space);

  bool valid_jpeg = true;

  JSAMPROW row_pointer[1];
  row_pointer[0] = static_cast<JSAMPLE*>(malloc(
      jpeg_decompress->output_width * jpeg_decompress->output_components));
  while (jpeg_compress_.next_scanline < jpeg_compress_.image_height) {
    const JDIMENSION num_scanlines_read =
        jpeg_read_scanlines(jpeg_decompress, row_pointer, 1);
    if (num_scanlines_read != 1) {
      valid_jpeg = false;
      break;
    }

    if (jpeg_write_scanlines(&jpeg_compress_, row_pointer, 1) != 1) {
      // We failed to write all the row. Abort.
      valid_jpeg = false;
      break;
    }
  }

  free(row_pointer[0]);
  return valid_jpeg;
}

bool JpegOptimizer::OptimizeLossless(
    jpeg_decompress_struct *jpeg_decompress,
    std::string *compressed,
    bool progressive) {
  jvirt_barray_ptr *coefficients = jpeg_read_coefficients(jpeg_decompress);
  bool valid_jpeg = (coefficients != NULL);

  if (valid_jpeg) {
    // Copy data from the source to the dest.
    jpeg_copy_critical_parameters(jpeg_decompress, &jpeg_compress_);

  if (progressive) {
    // Enable progressive jpeg if specified in the options.
    jpeg_simple_progression(&jpeg_compress_);
  }

    // Set optimize huffman to true.
    jpeg_compress_.optimize_coding = TRUE;

    // Prepare to write to a string.
    JpegStringWriter(&jpeg_compress_, compressed);

    // Copy the coefficients into the compression struct.
    jpeg_write_coefficients(&jpeg_compress_, coefficients);
  }

  return valid_jpeg;
}

// Helper for JpegOptimizer::CreateOptimizedJpeg().  This function does the
// work, and CreateOptimizedJpeg() does some cleanup.
bool JpegOptimizer::DoCreateOptimizedJpeg(
    const std::string &original,
    jpeg_decompress_struct *jpeg_decompress,
    std::string *compressed,
    const pagespeed::image_compression::JpegCompressionOptions *options) {
  // libjpeg's error handling mechanism requires that longjmp be used
  // to get control after an error.
  jmp_buf env;
  if (setjmp(env)) {
    // This code is run only when libjpeg hit an error, and called
    // longjmp(env).  Returning false will cause jpeg_abort_(de)compress to be
    // called on jpeg_(de)compress_, putting those structures back into a state
    // where they can be used again.
    return false;
  }

  // Need to install env so that it will be longjmp()ed to on error.
  jpeg_decompress->client_data = static_cast<void *>(&env);
  jpeg_compress_.client_data = static_cast<void *>(&env);

  reader_.PrepareForRead(original);

  // Read jpeg data into the decompression struct.
  jpeg_read_header(jpeg_decompress, TRUE);

  bool valid_jpeg = false;
  if (options != NULL && options->lossy) {
    valid_jpeg = OptimizeLossy(jpeg_decompress, compressed, options->quality,
                               options->progressive);
  } else {
    valid_jpeg = OptimizeLossless(jpeg_decompress, compressed,
                                  options != NULL && options->progressive);
  }

  // Finish the compression process.
  jpeg_finish_compress(&jpeg_compress_);
  jpeg_finish_decompress(jpeg_decompress);

  return valid_jpeg;
}

bool JpegOptimizer::CreateOptimizedJpeg(
    const std::string &original,
    std::string *compressed,
    const pagespeed::image_compression::JpegCompressionOptions *options) {
  jpeg_decompress_struct* jpeg_decompress = reader_.decompress_struct();

  bool result = DoCreateOptimizedJpeg(original, jpeg_decompress, compressed,
                                      options);

  jpeg_decompress->client_data = NULL;
  jpeg_compress_.client_data = NULL;

  if (!result) {
    // Clean up the state of jpeglib structures.  It is okay to abort even if
    // no (de)compression is in progress.  This is crucial because we enter
    // this block even if no jpeg-related error happened.
    jpeg_abort_decompress(jpeg_decompress);
    jpeg_abort_compress(&jpeg_compress_);
  }

  return result;
}

}  // namespace

namespace pagespeed {

namespace image_compression {

bool OptimizeJpeg(const std::string &original,
                  std::string *compressed) {
  JpegOptimizer optimizer;
  return optimizer.CreateOptimizedJpeg(original, compressed, NULL);
}

bool OptimizeJpegWithOptions(const std::string &original,
                             std::string *compressed,
                             const JpegCompressionOptions *options) {
  JpegOptimizer optimizer;
  return optimizer.CreateOptimizedJpeg(original, compressed, options);
}

}  // namespace image_compression

}  // namespace pagespeed
