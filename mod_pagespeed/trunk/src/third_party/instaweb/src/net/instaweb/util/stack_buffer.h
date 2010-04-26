// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

// Constant for allocating stack buffers.

#ifndef NET_INSTAWEB_UTIL_STACK_BUFFER_H_
#define NET_INSTAWEB_UTIL_STACK_BUFFER_H_

namespace net_instaweb {

// Size of stack buffer for read-blocks.  This can't be too big or it will blow
// the stack, which may be set small in multi-threaded environments.
const int kStackBufferSize = 10000;

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_STACK_BUFFER_H_
