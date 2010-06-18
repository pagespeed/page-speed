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

#include "pagespeed/http_content_decoder.h"

#include <vector>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "net/base/filter.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_request_job.h"

namespace {
static const size_t kBufferSize = 4096;
}  // namespace

namespace pagespeed {

HttpContentDecoder::HttpContentDecoder(URLRequestJob* job,
                                       const std::string& encoded_body)
    : job_(job),
      encoded_body_(encoded_body),
      encoded_body_pos_(0),
      buf_(new char[kBufferSize]) {
}

HttpContentDecoder::~HttpContentDecoder() {
}

bool HttpContentDecoder::NeedsDecoding() {
  std::vector<Filter::FilterType> encoding_types;
  return job_->GetContentEncodings(&encoding_types);
}

bool HttpContentDecoder::Decode(std::string* out_decoded_body) {
  std::vector<Filter::FilterType> encoding_types;
  if (!job_->GetContentEncodings(&encoding_types)) {
    return false;
  }

  scoped_ptr<Filter> filter(Filter::Factory(encoding_types, *job_));
  encoded_body_pos_ = 0;

  // We really want to loop indefinitely here, but just in case there
  // is a bug in the decoding code, we limit the number of loop
  // iterations and assume that if we exceed that number, we were in
  // an infinite loop.
  for (int i = 0; i < 10000; ++i) {
    if (!MaybePushEncodedDataIntoFilter(filter.get())) {
      return false;
    }

    switch (PullDecodedDataFromFilter(filter.get(), out_decoded_body)) {
      case DECODE_CONTINUE:
        break;

      case DECODE_DONE:
        return true;

      case DECODE_ERROR:
        return false;

      default:
        LOG(DFATAL) << "Unexpected PullDecodedDataFromFilter result.";
        return false;
    }
  }

  LOG(DFATAL) << "Reached unexpected point in Decode(). Too many iterations?";
  return false;
}

bool HttpContentDecoder::MaybePushEncodedDataIntoFilter(Filter* filter) {
  if (filter->stream_data_len() != 0) {
    // There's already data in the buffer, so we don't need to push
    // additional data in at this time.
    return true;
  }

  net::IOBuffer* stream_buffer = filter->stream_buffer();
  const int stream_buffer_size = filter->stream_buffer_size();
  int num_to_read = encoded_body_.size() - encoded_body_pos_;
  if (num_to_read > stream_buffer_size) {
    num_to_read = stream_buffer_size;
  }
  int num_read = encoded_body_.copy(stream_buffer->data(),
                                    num_to_read,
                                    encoded_body_pos_);

  if (!filter->FlushStreamBuffer(num_read)) {
    LOG(INFO) << "Failed to FlushStreamBuffer";
    return false;
  }
  encoded_body_pos_ += num_read;
  return true;
}

HttpContentDecoder::DecodeResult HttpContentDecoder::PullDecodedDataFromFilter(
    Filter* filter,
    std::string* out_decoded_body) {
  int buf_len = kBufferSize;
  switch (filter->ReadData(buf_.get(), &buf_len)) {
    case Filter::FILTER_DONE:
      out_decoded_body->append(buf_.get(), buf_len);
      return DECODE_DONE;

    case Filter::FILTER_NEED_MORE_DATA:
    case Filter::FILTER_OK:
      out_decoded_body->append(buf_.get(), buf_len);
      return DECODE_CONTINUE;

    case Filter::FILTER_ERROR:
      LOG(INFO) << "Failed to ReadData";
      out_decoded_body->clear();
      return DECODE_ERROR;

    default:
      LOG(DFATAL) << "Unexpected ReadData result.";
      out_decoded_body->clear();
      return DECODE_ERROR;
  }
}

}  // namespace pagespeed
