// Copyright 2010 Google Inc. All Rights Reserved.
// Author: sligocki@google.com (Shawn Ligocki)
//
// A collection of content-types and their attributes.

#ifndef NET_INSTAWEB_UTIL_PUBLIC_CONTENT_TYPE_H_
#define NET_INSTAWEB_UTIL_PUBLIC_CONTENT_TYPE_H_

#include <assert.h>

namespace net_instaweb {

struct ContentType {
 public:
  const char* mime_type() const { return mime_type_; }
  const char* file_extension() const { return file_extension_; }

  const char* mime_type_;
  const char* file_extension_;
};

extern const ContentType kContentTypeJavascript;
extern const ContentType kContentTypeCss;

extern const ContentType kContentTypePng;
extern const ContentType kContentTypeGif;
extern const ContentType kContentTypeJpeg;

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_CONTENT_TYPE_H_
