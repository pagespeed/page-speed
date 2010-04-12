// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//
// Meta-data associated with a rewriting resource.  This is
// primarily a key-value store, but additionally we want to
// get easy access to the cache expiration time.

#include "net/instaweb/util/public/meta_data.h"

namespace net_instaweb {

MetaData::~MetaData() {
}
}
