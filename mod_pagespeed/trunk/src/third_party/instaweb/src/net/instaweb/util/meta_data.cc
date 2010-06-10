// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/meta_data.h"

namespace net_instaweb {

MetaData::~MetaData() {
}

void MetaData::CopyFrom(const MetaData& other) {
  set_major_version(other.major_version());
  set_minor_version(other.minor_version());
  set_status_code(other.status_code());
  set_reason_phrase(other.reason_phrase());
  for (int i = 0; i < other.NumAttributes(); ++i) {
    Add(other.Name(i), other.Value(i));
  }
  ComputeCaching();
}

}  // namespace net_instaweb
