// Copyright 2010 Google Inc. All Rights Reserved.
// Author: sligocki@google.com (Shawn Ligocki)
//
// A collection of content-types and their attributes.

#ifndef NET_INSTAWEB_UTIL_PUBLIC_CONTENT_TYPE_H_
#define NET_INSTAWEB_UTIL_PUBLIC_CONTENT_TYPE_H_

#include <assert.h>

namespace net_instaweb {

class ContentType {
 public:
  ContentType(const char* mime_type, const char* file_extension)
      : mime_type_(mime_type), file_extension_(file_extension) {}

  const char* mime_type() const { return mime_type_; }
  const char* file_extension() const { return file_extension_; }

 private:
  const char* mime_type_;
  const char* file_extension_;
};

const ContentType kContentTypeJavascript("text/javascript", ".js");
const ContentType kContentTypeCss("text/css", ".css");

const ContentType kContentTypePng("image/png", ".png");
const ContentType kContentTypeGif("image/gif", ".gif");
const ContentType kContentTypeJpeg("image/jpeg", ".jpg");

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_CONTENT_TYPE_H_
