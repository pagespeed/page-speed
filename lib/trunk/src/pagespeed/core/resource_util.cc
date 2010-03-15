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

#include "pagespeed/core/resource_util.h"

#include "base/logging.h"
#include "base/string_tokenizer.h"
#include "pagespeed/core/resource.h"

namespace {

// each message header has a 3 byte overhead; colon between the key
// value pair and the end-of-line CRLF.
const int kHeaderOverhead = 3;

int EstimateHeadersBytes(const std::map<std::string, std::string>& headers) {
  int total_size = 0;

  // TODO improve the header size calculation below.
  for (std::map<std::string, std::string>::const_iterator
           iter = headers.begin(), end = headers.end();
       iter != end;
       ++iter) {
    total_size += kHeaderOverhead + iter->first.size() + iter->second.size();
  }

  return total_size;
}

// Enumerates HTTP header directives.
class DirectiveEnumerator {
 public:
  explicit DirectiveEnumerator(const std::string& header);

  bool GetNext(std::string* key, std::string* value);

  bool done() const { return state_ == STATE_DONE; }
  bool error() const { return state_ == STATE_ERROR; }

 private:
  enum State {
    STATE_START,
    CONSUMED_KEY,
    CONSUMED_EQ,
    CONSUMED_VALUE,
    STATE_DONE,
    STATE_ERROR,
  };

  bool CanTransition(State src, State dest) const;
  bool Transition(State dest);

  bool GetNextInternal(std::string* key, std::string* value);
  bool OnDelimiter(char c);
  bool OnToken(std::string* key, std::string* value);

  std::string header_;
  StringTokenizer tok_;
  State state_;
};

DirectiveEnumerator::DirectiveEnumerator(const std::string& header)
    : header_(header),
      tok_(header_, ",; ="),
      state_(STATE_START) {
  tok_.set_quote_chars("\"");
  tok_.set_options(StringTokenizer::RETURN_DELIMS);
}

bool DirectiveEnumerator::CanTransition(State src, State dest) const {
  if (dest == STATE_ERROR) {
    return src != STATE_ERROR;
  }
  if (dest == STATE_DONE) {
    return src != STATE_ERROR && src != STATE_DONE;
  }
  switch (src) {
    case STATE_START:
      return dest == CONSUMED_KEY ||
          // Allow headers like "foo,,," or "foo,,,bar".
          dest == STATE_START;
    case CONSUMED_KEY:
      return dest == CONSUMED_EQ || dest == STATE_START;
    case CONSUMED_EQ:
      return dest == CONSUMED_VALUE ||
          // Allow headers like "foo==" or "foo==bar".
          dest == CONSUMED_EQ ||
          // Allow headers like "foo=," or "foo=,bar".
          dest == STATE_START;
    case CONSUMED_VALUE:
      return dest == STATE_START;
    case STATE_DONE:
      return false;
    case STATE_ERROR:
      return false;
    default:
      DCHECK(false);
      return false;
  }
}

bool DirectiveEnumerator::Transition(State dest) {
  if (!CanTransition(state_, dest)) {
    return false;
  }
  state_ = dest;
  return true;
}

bool DirectiveEnumerator::GetNext(std::string* key, std::string* value) {
  if (error() || done()) {
    return false;
  }

  if (state_ != STATE_START) {
    LOG(DFATAL) << "Unexpected state " << state_;
    Transition(STATE_ERROR);
    return false;
  }

  key->clear();
  value->clear();
  if (!GetNextInternal(key, value)) {
    Transition(STATE_ERROR);
    key->clear();
    value->clear();
    return false;
  }

  if (done()) {
    // Special case: if we're at end-of-stream, only return true if we
    // found a key. This covers cases where we get a header like
    // "foo,".
    return !key->empty();
  }

  return done() || Transition(STATE_START);
}

bool DirectiveEnumerator::GetNextInternal(std::string* key,
                                          std::string* value) {
  if (error() || done()) {
    LOG(DFATAL) << "Terminal state " << state_;
    return false;
  }

  if (!tok_.GetNext()) {
    // end-of-stream
    return Transition(STATE_DONE);
  }

  if (tok_.token_is_delim()) {
    if (!OnDelimiter(*tok_.token_begin())) {
      return false;
    }
    // Check to see if we've parsed a full directive. If so, return.
    if (!key->empty() && state_ == STATE_START) {
      return true;
    }
  } else {
    if (!OnToken(key, value)) {
      return false;
    }
  }

  return GetNextInternal(key, value);
}

bool DirectiveEnumerator::OnDelimiter(char c) {
  switch (c) {
    case ' ':
      // skip whitespace
      return true;
    case '=':
      return Transition(CONSUMED_EQ);
    case ',':
    case ';':
      return Transition(STATE_START);
    default:
      return false;
  }
}

bool DirectiveEnumerator::OnToken(std::string* key, std::string* value) {
  switch (state_) {
    case STATE_START:
      *key = tok_.token();
      if (key->find_first_of('\"') != key->npos) {
        // keys are not allowed to be quoted.
        return false;
      }
      return Transition(CONSUMED_KEY);
    case CONSUMED_EQ:
      *value = tok_.token();
      return Transition(CONSUMED_VALUE);
    default:
      return false;
  }
}

}  // namespace

namespace pagespeed {

namespace resource_util {

int EstimateRequestBytes(const Resource& resource) {
  int request_bytes = 0;

  // Request line
  request_bytes += resource.GetRequestMethod().size() + 1 /* space */ +
      resource.GetRequestUrl().size()  + 1 /* space */ +
      resource.GetRequestProtocol().size() + 2 /* \r\n */;

  request_bytes += EstimateHeadersBytes(*resource.GetRequestHeaders());
  request_bytes += resource.GetRequestBody().size();

  return request_bytes;
}

int EstimateResponseBytes(const Resource& resource) {
  int response_bytes = 0;
  // TODO get compressed size or replace with section with actual
  // download size.
  // TODO improve the header size calculation below.
  response_bytes += resource.GetResponseBody().size();
  response_bytes += resource.GetResponseProtocol().size();
  response_bytes += EstimateHeadersBytes(*resource.GetResponseHeaders());
  return response_bytes;
}

bool GetHeaderDirectives(const std::string& header, DirectiveMap* out) {
  DirectiveEnumerator e(header);
  std::string key;
  std::string value;
  while (e.GetNext(&key, &value)) {
    if (key.empty()) {
      LOG(DFATAL) << "Received empty key.";
      out->clear();
      return false;
    }
    (*out)[key] = value;
  }
  if (!e.error() && !e.done()) {
    LOG(DFATAL) << "Failed to reach terminal state.";
    out->clear();
    return false;
  }
  if (e.error()) {
    out->clear();
    return false;
  }
  return true;
}

}  // namespace resource_util

}  // namespace pagespeed
