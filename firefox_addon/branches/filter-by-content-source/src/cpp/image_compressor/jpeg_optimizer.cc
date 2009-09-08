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

// Author: Bryan McQuade

#include "jpeg_optimizer.h"

#include <setjmp.h>  // for setjmp/longjmp
#include <string.h>  // for memset

namespace pagespeed {

JpegOptimizer::JpegOptimizer() {
}

JpegOptimizer::~JpegOptimizer() {
}

// ErrorExit() is installed as a callback, called on errors
// encountered within libjpeg.  The longjmp jumps back
// to the setjmp in JpegOptimizer::CreateOptimizedJpeg().
static void ErrorExit(j_common_ptr jpeg_state_struct) {
  jmp_buf *env = static_cast<jmp_buf *>(jpeg_state_struct->client_data);
  (*jpeg_state_struct->err->output_message)(jpeg_state_struct);
  if (env)
    longjmp(*env, 1);
}

// OutputMessageFromReader is called by libjpeg code on an error when reading.
// Without this function, a default function would print to standard error.
static void OutputMessage(j_common_ptr jpeg_decompress) {
  // The following code is handy for debugging.
  /*
  char buf[JMSG_LENGTH_MAX];
  (*jpeg_decompress->err->format_message)(jpeg_decompress, buf);
  cerr << "JPEG Reader Error: " << buf << endl;
  */
}

bool JpegOptimizer::Initialize() {
  memset(&jpeg_decompress_, 0, sizeof(jpeg_decompress_struct));
  memset(&jpeg_compress_, 0, sizeof(jpeg_compress_struct));
  memset(&decompress_error_, 0, sizeof(jpeg_error_mgr));
  memset(&compress_error_, 0, sizeof(jpeg_error_mgr));

  jpeg_decompress_.err = jpeg_std_error(&decompress_error_);
  decompress_error_.error_exit = &ErrorExit;
  decompress_error_.output_message = &OutputMessage;
  jpeg_create_decompress(&jpeg_decompress_);

  jpeg_compress_.err = jpeg_std_error(&compress_error_);
  compress_error_.error_exit = &ErrorExit;
  compress_error_.output_message = &OutputMessage;
  jpeg_create_compress(&jpeg_compress_);

  jpeg_compress_.optimize_coding = TRUE;

  return true;
}

bool JpegOptimizer::Finalize() {
  jpeg_destroy_compress(&jpeg_compress_);
  jpeg_destroy_decompress(&jpeg_decompress_);

  return true;
}

// Helper for JpegOptimizer::CreateOptimizedJpeg().  This function does the
// work, and CreateOptimizedJpeg() does some cleanup.
bool JpegOptimizer::DoCreateOptimizedJpeg(
    const char *infile, const char *outfile) {
  FILE *fout = NULL;
  FILE *fin = fopen(infile, "rb");
  if (fin == NULL) {
    return false;
  }

  // libjpeg's error handling mechanism requires that longjmp be used
  // to get control after an error.
  jmp_buf env;
  if (setjmp(env)) {
    // This code is run only when libjpeg hit an error, and called longjmp(env).
    // Close any open file handles.
    if (fin)
      fclose(fin);

    if (fout)
      fclose(fout);

    // Returning false will cause jpeg_abort_(de)compress to be called on
    // jpeg_(de)compress_, putting those structures back into a state where
    // they can be used again.

    return false;
  }

  // Need to install env so that it will be longjmp()ed to on error.
  jpeg_decompress_.client_data = static_cast<void *>(&env);
  jpeg_compress_.client_data = static_cast<void *>(&env);

  jpeg_stdio_src(&jpeg_decompress_, fin);

  // Read jpeg data into the decompression struct.
  jpeg_read_header(&jpeg_decompress_, TRUE);
  jvirt_barray_ptr *coefficients = jpeg_read_coefficients(&jpeg_decompress_);

  // Copy data from the source to the dest.
  jpeg_copy_critical_parameters(&jpeg_decompress_, &jpeg_compress_);

  // Close the input file. It's important that we do this before
  // opening the output file, in case they're the same file.
  fclose(fin);
  fin = NULL;  // Error handling code will re-close fin if it is not NULL.

  // Open the output file and wire it up to the compression struct.
  fout = fopen(outfile, "wb");
  if (fout == NULL) {
    return false;
  }

  jpeg_stdio_dest(&jpeg_compress_, fout);

  // Copy the coefficients into the compression struct.
  jpeg_write_coefficients(&jpeg_compress_, coefficients);

  // Finish the compression process.
  jpeg_finish_compress(&jpeg_compress_);
  jpeg_finish_decompress(&jpeg_decompress_);

  // Close the output file.
  fclose(fout);

  return true;
}

bool JpegOptimizer::CreateOptimizedJpeg(
    const char *infile, const char *outfile) {

  bool result = DoCreateOptimizedJpeg(infile, outfile);

  jpeg_decompress_.client_data = NULL;
  jpeg_compress_.client_data = NULL;

  if (!result) {
    // Clean up the state of jpeglib structures.
    // It is okay to abort even if no (de)compression is in progress.
    // This is crucial because we enter this block even if no jpeg-related error
    // happened.  For example, if fopen() fails to open a file,
    // DoCreateOptimizedJpeg() returns false.
    jpeg_abort_decompress(&jpeg_decompress_);
    jpeg_abort_compress(&jpeg_compress_);
  }

  return result;
}

}  // namespace pagespeed
