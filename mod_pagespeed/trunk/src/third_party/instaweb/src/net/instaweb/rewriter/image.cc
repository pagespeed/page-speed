// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#include "net/instaweb/rewriter/public/image.h"

#include "net/instaweb/rewriter/public/input_resource.h"
#include "net/instaweb/rewriter/public/resource_manager.h"
#include "net/instaweb/util/public/content_type.h"
#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/message_handler.h"
#include <string>
#include "net/instaweb/util/public/string_util.h"
#include "net/instaweb/util/public/writer.h"
#define PAGESPEED_PNG_OPTIMIZER_GIF_READER 0
#include "pagespeed/image_compression/gif_reader.h"
#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/png_optimizer.h"

namespace net_instaweb {

namespace {
const char kPngHeader[] = "\x89PNG\r\n\x1a\n";
const int kPngHeaderLength = sizeof(kPngHeader) - 1;

const char kGifHeader[] = "GIF8";
const int kGifHeaderLength = sizeof(kGifHeader) - 1;

bool WriteTempFileWithContentType(
    const StringPiece& prefix_name, const ContentType& content_type,
    const std::string& buffer, std::string* filename,
    FileSystem* file_system, MessageHandler* handler) {
  std::string tmp_filename;
  bool ok = file_system->WriteTempFile(
      prefix_name.as_string().c_str(), buffer, &tmp_filename, handler);
  if (ok) {
    *filename = StrCat(tmp_filename, content_type.file_extension());
    ok = file_system->RenameFile(
        tmp_filename.c_str(), filename->c_str(), handler);
  }
  return ok;
}
}  // namespace

Image::Image(const InputResource& original_image,
             FileSystem* file_system,
             ResourceManager* manager, MessageHandler* handler)
    : file_system_(file_system),
      handler_(handler),
      original_image_(original_image),
      manager_(manager),
      image_type_(IMAGE_UNKNOWN),
      output_contents_(),
      output_valid_(false),
      opencv_filename_(),
      opencv_image_(NULL),
      opencv_load_possible_(false),
      resized_(false) {
}

void Image::ComputeImageType() {
  // Image classification based on buffer contents gakked from leptonica,
  // but based on well-documented headers (see Wikipedia etc.).
  // Note that we can be fooled if we're passed random binary data;
  // we make the call based on as few as two bytes (JPEG)
  const std::string& buf = original_contents();
  if (buf.size() >= 8) {
    // Note that gcc rightly complains about constant ranges with the
    // negative char constants unless we cast.
    switch (static_cast<unsigned char>(buf[0])) {
      case 0xff:
        // either jpeg or jpeg2
        // (the latter we don't handle yet, and don't bother looking for).
        if (static_cast<unsigned char>(buf[1]) == 0xd8) {
          image_type_ = IMAGE_JPEG;
        }
        break;
      case 0x89:
        // possible png
        if (buf.compare(0, kPngHeaderLength, kPngHeader) == 0) {
          image_type_ = IMAGE_PNG;
        }
        break;
      case 'G':
        // possible gif
        if (buf.compare(0, kGifHeaderLength, kGifHeader) == 0 &&
            (buf[4] == '7' || buf[4] == '9') && buf[5] == 'a') {
          image_type_ = IMAGE_GIF;
        }
        break;
      default:
        break;
    }
  }
}

const ContentType* Image::content_type() {
  const ContentType* res = NULL;
  switch (image_type()) {
    case IMAGE_UNKNOWN:
      break;
    case IMAGE_JPEG:
      res = &kContentTypeJpeg;
      break;
    case IMAGE_PNG:
      res = &kContentTypePng;
      break;
    case IMAGE_GIF:
      res = &kContentTypeGif;
      break;
  }
  return res;
}


// Make sure OpenCV version of image is loaded if that is possible.
// Returns value of opencv_load_possible_ after load attempted.
// Note that if the load fails, opencv_load_possible_ will be false
// and future calls to LoadOpenCV will fail fast.
bool Image::LoadOpenCV() {
  return opencv_load_possible_;
}

void Image::CleanOpenCV() {
}

bool Image::Dimensions(int* width, int* height) {
  bool ok = false;
  return ok;
}

bool Image::ResizeTo(int width, int height) {
  return resized_;
}

// Perform image optimization and output
bool Image::ComputeOutputContents() {
  if (!output_valid_) {
    bool ok = true;
    std::string opencv_contents;
    const std::string* contents = &original_contents();
    // Take image contents and re-compress them.
    if (ok) {
      // If we can't optimize the image, we'll fail.
      ok = false;
      switch (image_type()) {
        case IMAGE_UNKNOWN:
          break;
        case IMAGE_JPEG:
          ok = pagespeed::image_compression::OptimizeJpeg(*contents,
                                                          &output_contents_);
          break;
        case IMAGE_PNG: {
          pagespeed::image_compression::PngReader png_reader;
          ok = pagespeed::image_compression::PngOptimizer::OptimizePng(
              png_reader, *contents, &output_contents_);
          break;
        }
        case IMAGE_GIF: {
#if PAGESPEED_PNG_OPTIMIZER_GIF_READER
          pagespeed::image_compression::GifReader gif_reader;
          ok = pagespeed::image_compression::PngOptimizer::OptimizePng(
              gif_reader, *contents, &output_contents_);
          if (ok) {
            image_type_ = IMAGE_PNG;
          }
#endif
          break;
        }
      }
    }
    output_valid_ = ok;
  }
  return output_valid_;
}

bool Image::WriteTo(Writer* writer) {
  bool ok = false;
  const ContentType* content_type = this->content_type();
  if (content_type != NULL) {
    const std::string* contents = &original_contents();
    if (output_valid_ || ComputeOutputContents()) {
      contents = &output_contents_;
    }
    ok = writer->Write(*contents, handler_);
  }
  return ok;
}

std::string Image::AsInlineData() {
  std::string result = "data:...TODO...";
  // TODO(jmaessen): finish this so we can use it.
  return result;
}

const ContentType* NameExtensionToContentType(const StringPiece& name) {
  const ContentType* res = NULL;
  if (name.ends_with(kContentTypeJpeg.file_extension())) {
    res = &kContentTypeJpeg;
  } else if (name.ends_with(kContentTypePng.file_extension())) {
    res = &kContentTypePng;
  } else if (name.ends_with(kContentTypeGif.file_extension())) {
    res = &kContentTypeGif;
  }
  return res;
}

}  // namespace net_instaweb
