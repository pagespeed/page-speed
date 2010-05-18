// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_PROTO_UTIL_H_
#define NET_INSTAWEB_UTIL_PUBLIC_PROTO_UTIL_H_

#include "google/protobuf/io/gzip_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

namespace net_instaweb {

typedef google::protobuf::io::StringOutputStream StringOutputStream;
typedef google::protobuf::io::GzipOutputStream GzipOutputStream;
typedef google::protobuf::io::ArrayInputStream ArrayInputStream;
typedef google::protobuf::io::GzipInputStream GzipInputStream;

}  // namespace net_instaweb


#endif  // NET_INSTAWEB_UTIL_PUBLIC_PROTO_UTIL_H_
