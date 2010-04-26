// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/input_resource.h"

namespace net_instaweb {

InputResource::~InputResource() {
}

const InputResource::ImageType InputResource::image_type() const {
  // Image classification based on buffer contents gakked from leptonica,
  // but based on well-documented headers (see Wikipedia etc.).
  // Note that we can be fooled if we're passed random binary data;
  // we make the call based on as few as two bytes (JPEG)
  InputResource::ImageType res = InputResource::IMAGE_UNKNOWN;
  const std::string& buf = contents();
  if (buf.size() >= 8) {
    switch (static_cast<unsigned char>(buf[0])) {
      case 0xff:
        // either jpeg or jpeg2
        // (the latter we don't handle yet, and don't bother looking for).
        // Note that gcc complains about constant ranges with the
        // negative char constant in the following test unless we cast.
        if (static_cast<unsigned char>(buf[1]) == 0xd8) {
          res = InputResource::IMAGE_JPEG;
        }
        break;
      case 0x89:
        // possible png
        if (buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G' &&
            buf[4] == '\r' && buf[5] == '\n' && buf[6] == 0x1a &&
            buf[7] == '\n') {
          res = InputResource::IMAGE_PNG;
        }
        break;
      case 'G':
        // possible gif
        if (buf[1] == 'I' && buf[2] == 'F' && buf[3] == '8' &&
            (buf[4] == '7' || buf[4] == '9') && buf[5] == 'a') {
          res = InputResource::IMAGE_GIF;
        }
        break;
      default:
        break;
    }
  }
  return res;
}

}  // namespace net_instaweb
