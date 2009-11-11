// Copyright 2009 Google Inc. All Rights Reserved.
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

#include "pagespeed/rules/enable_gzip_compression.h"

#include <string>

#include "base/logging.h"
#include "base/string_util.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "third_party/zlib/zlib.h"

namespace pagespeed {

namespace rules {

EnableGzipCompression::EnableGzipCompression(SavingsComputer* computer)
    : computer_(computer) {
  CHECK(NULL != computer) << "SavingsComputer must be non-null.";
}

const char* EnableGzipCompression::name() const {
  return "EnableGzipCompression";
}

const char* EnableGzipCompression::header() const {
  return "Enable gzip compression";
}

bool EnableGzipCompression::AppendResults(const PagespeedInput& input,
                                          Results* results) {
  bool success = true;
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    int bytes_saved = 0;
    bool resource_success = GetSavings(resource, &bytes_saved);
    if (!resource_success) {
      success = false;
      continue;
    }

    if (bytes_saved <= 0) {
      continue;
    }

    Result* result = results->add_results();
    result->set_rule_name(name());

    Savings* savings = result->mutable_savings();
    savings->set_response_bytes_saved(bytes_saved);

    result->add_resource_urls(resource.GetRequestUrl());
  }

  return success;
}

void EnableGzipCompression::FormatResults(
    const ResultVector& results, Formatter* formatter) {
  int total_bytes_saved = 0;

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    const Savings& savings = result.savings();
    total_bytes_saved += savings.response_bytes_saved();
  }

  Argument arg(Argument::BYTES, total_bytes_saved);
  Formatter* body = formatter->AddChild("Compressing the following "
                                        "resources with gzip could reduce "
                                        "their transfer size by $1.",
                                        arg);

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    CHECK(result.resource_urls_size() == 1);
    Argument url(Argument::URL, result.resource_urls(0));
    Argument savings(Argument::BYTES, result.savings().response_bytes_saved());
    body->AddChild("Compressing $1 could save $2.", url, savings);
  }
}

bool EnableGzipCompression::IsCompressed(const Resource& resource) const {
  const std::string& encoding = resource.GetResponseHeader("Content-Encoding");

  // HTTP allows Content-Encodings to be "stacked" in which case they
  // are comma-separated. Instead of splitting on commas and checking
  // each token, we just see if a valid known encoding appears in the
  // header, and if so, assume that encoding was applied to the
  // response.
  return encoding.find("gzip") != std::string::npos ||
      encoding.find("deflate") != std::string::npos;
}

bool EnableGzipCompression::IsText(const Resource& resource) const {
  ResourceType type = resource.GetResourceType();
  ResourceType text_types[] = { HTML, TEXT, JS, CSS };
  for (int idx = 0; idx < arraysize(text_types); ++idx) {
    if (type == text_types[idx]) {
      return true;
    }
  }

  return false;
}

bool EnableGzipCompression::IsViolation(const Resource& resource) const {
  return !IsCompressed(resource) &&
      IsText(resource) &&
      resource.GetResponseBody().size() >= 150;
}

bool EnableGzipCompression::GetSavings(const Resource& resource,
                                       int* out_savings) const {
  *out_savings = 0;
  if (!IsViolation(resource)) {
    return true;
  }

  Savings savings;
  bool result = computer_->ComputeSavings(resource, &savings);
  *out_savings = savings.response_bytes_saved();
  return result;
}

namespace compression_computer {

bool ZlibComputer::ComputeSavings(const pagespeed::Resource& resource,
                                  pagespeed::Savings* savings) {
  z_stream c_stream; /* compression stream */
  c_stream.zalloc = (alloc_func)0;
  c_stream.zfree = (free_func)0;
  c_stream.opaque = (voidpf)0;

  int err = deflateInit2(
      &c_stream,
      Z_DEFAULT_COMPRESSION,
      Z_DEFLATED,
      31,  // window size of 15, plus 16 for gzip
      8,   // default mem level (no zlib constant exists for this value)
      Z_DEFAULT_STRATEGY);
  if (err != Z_OK) {
    LOG(WARNING) << "Failed to deflateInit2: " << err;
    return false;
  }

  c_stream.next_in = reinterpret_cast<Bytef*>(
      const_cast<char*>(resource.GetResponseBody().data()));
  c_stream.avail_in = resource.GetResponseBody().size();

  int compressed_size = 0;
  bool result = GetCompressedSize(&c_stream, &compressed_size);

  // clean up.
  err = deflateEnd(&c_stream);
  if (err != Z_OK) {
    LOG(WARNING) << "Failed to deflateEnd: " << err;
    return false;
  }

  savings->set_response_bytes_saved(
      resource.GetResponseBody().size() - compressed_size);
  return result;
}


bool ZlibComputer::GetCompressedSize(z_stream* c_stream, int* compressed_size) {
  scoped_array<char> buffer(new char[kBufferSize]);

  int err = Z_OK;
  bool finished = false;

  while (!finished) {
    c_stream->next_out = reinterpret_cast<Bytef*>(buffer.get());
    c_stream->avail_out = kBufferSize;
    err = deflate(c_stream, Z_FINISH);

    switch (err) {
      case Z_OK:
        break;

      case Z_STREAM_END:
        finished = true;
        break;

      default:
        LOG(WARNING) << "GetCompressedSize encountered error: " << err;
        return false;
    }

    *compressed_size += (kBufferSize - c_stream->avail_out);
  }

  const bool success = (err == Z_STREAM_END);
  if (!success) {
    LOG(WARNING) << "GetCompressedSize expected Z_STREAM_END, got " << err;
  }
  return success;
}

}  // namespace compression_computer

}  // namespace rules

}  // namespace pagespeed
