// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_PROTO_UTIL_H_
#define NET_INSTAWEB_UTIL_PUBLIC_PROTO_UTIL_H_

#ifdef INSTAWEB_GOOGLE3
#include "net/proto2/io/public/gzip_stream.h"
#include "net/proto2/io/public/zero_copy_stream_impl_lite.h"

namespace net_instaweb {

typedef proto2::io::StringOutputStream StringOutputStream;
typedef proto2::io::GzipOutputStream GzipOutputStream;
typedef proto2::io::ArrayInputStream ArrayInputStream;
typedef proto2::io::GzipInputStream GzipInputStream;

}  // namespace net_instaweb

#else
#include "google/protobuf/io/gzip_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

namespace net_instaweb {

typedef google::protobuf::io::StringOutputStream StringOutputStream;
typedef google::protobuf::io::GzipOutputStream GzipOutputStream;
typedef google::protobuf::io::ArrayInputStream ArrayInputStream;
typedef google::protobuf::io::GzipInputStream GzipInputStream;

}  // namespace net_instaweb

#endif


#endif  // NET_INSTAWEB_UTIL_PUBLIC_PROTO_UTIL_H_
