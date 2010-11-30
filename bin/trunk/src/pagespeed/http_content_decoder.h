// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PAGESPEED_HTTP_CONTENT_DECODER_H_
#define PAGESPEED_HTTP_CONTENT_DECODER_H_

#include <string>

#include "base/scoped_ptr.h"

class Filter;

namespace net {
class URLRequestJob;
}  // namespace net

namespace pagespeed {

class HttpContentDecoder {
 public:
  // URLRequestJob should be const, but
  // URLRequestJob::GetContentEncodings() is a non-const method so we are
  // forced to take a non-const pointer to the URLRequestJob.
  HttpContentDecoder(net::URLRequestJob* job,
                     const std::string& encoded_body);
  ~HttpContentDecoder();

  // Does the response associated with the given URLRequestJob need to
  // be decoded?
  bool NeedsDecoding();

  // Decode the encoded_body. This method should only be called if
  // NeedsDecoding() returns true.
  bool Decode(std::string* out_decoded_body);

 private:
  enum DecodeResult {
    DECODE_CONTINUE,
    DECODE_DONE,
    DECODE_ERROR,
  };

  bool MaybePushEncodedDataIntoFilter(Filter* filter);
  DecodeResult PullDecodedDataFromFilter(Filter* filter,
                                         std::string* out_decoded_body);

  net::URLRequestJob *const job_;
  std::string encoded_body_;
  int encoded_body_pos_;
  const scoped_array<char> buf_;
};

}  // namespace pagespeed

#endif  // PAGESPEED_HTTP_CONTENT_DECODER_H_
