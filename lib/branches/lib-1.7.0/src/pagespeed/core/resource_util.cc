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
#include "base/string_util.h"
#include "base/third_party/nspr/prtime.h"
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

bool IsHeuristicallyCacheable(const pagespeed::Resource& resource) {
  if (pagespeed::resource_util::HasExplicitFreshnessLifetime(resource)) {
    // If the response has an explicit freshness lifetime then it's
    // not heuristically cacheable. This method only expects to be
    // called if the resource does *not* have an explicit freshness
    // lifetime.
    LOG(DFATAL) << "IsHeuristicallyCacheable received a resource with "
                << "explicit freshness lifetime.";
    return false;
  }

  pagespeed::resource_util::DirectiveMap cache_directives;
  if (!pagespeed::resource_util::GetHeaderDirectives(
          resource.GetResponseHeader("Cache-Control"),
          &cache_directives)) {
    LOG(ERROR) << "Failed to parse cache control directives for "
               << resource.GetRequestUrl();
    return false;
  }

  if (cache_directives.find("must-revalidate") != cache_directives.end()) {
    // must-revalidate indicates that a non-fresh response should not
    // be used in response to requests without validating at the
    // origin. Such a resource is not heuristically cacheable.
    return false;
  }

  const std::string& url = resource.GetRequestUrl();
  if (url.find_first_of('?') != url.npos) {
    // The HTTP RFC says:
    //
    // ...since some applications have traditionally used GETs and
    // HEADs with query URLs (those containing a "?" in the rel_path
    // part) to perform operations with significant side effects,
    // caches MUST NOT treat responses to such URIs as fresh unless
    // the server provides an explicit expiration time.
    //
    // So if we find a '?' in the URL, the resource is not
    // heuristically cacheable.
    //
    // In practice most browsers do not implement this policy. For
    // instance, Chrome and IE8 do not look for the query string,
    // while Firefox (as of version 3.6) does. For the time being we
    // implement the RFC but it might make sense to revisit this
    // decision in the future, given that major browser
    // implementations do not match.
    return false;
  }

  if (!pagespeed::resource_util::IsCacheableResourceStatusCode(
          resource.GetResponseStatusCode())) {
    return false;
  }

  return true;
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
  // TODO: this computation is a bit strange. It mixes the size of
  // uncompressed response headers with uncompressed response
  // body. The computed number doesn't reflect an actual attribute of
  // the resource. If we want to compute the wire transfer size we
  // should be using the post-gzip compression size for this resource,
  // but it's not clear that it's necessarily the right thing to use
  // the gzip-compressed size.
  response_bytes += resource.GetResponseBody().size();
  response_bytes += resource.GetResponseProtocol().size();
  response_bytes += EstimateHeadersBytes(*resource.GetResponseHeaders());
  return response_bytes;
}

bool IsCompressibleResource(const Resource& resource) {
  switch (resource.GetResourceType()) {
    case HTML:
    case TEXT:
    case CSS:
    case JS:
      return true;

    default:
      return false;
  }
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

bool HasExplicitNoCacheDirective(const Resource& resource) {
  DirectiveMap cache_directives;
  if (!GetHeaderDirectives(resource.GetResponseHeader("Cache-Control"),
                           &cache_directives)) {
    LOG(ERROR) << "Failed to parse cache control directives for "
               << resource.GetRequestUrl();
    return true;
  }

  if (cache_directives.find("no-cache") != cache_directives.end()) {
    return true;
  }
  if (cache_directives.find("no-store") != cache_directives.end()) {
    return true;
  }
  DirectiveMap::const_iterator it = cache_directives.find("max-age");
  if (it != cache_directives.end()) {
    int64 max_age_value = 0;
    if (StringToInt64(it->second, &max_age_value) &&
        max_age_value == 0) {
      // Cache-Control: max-age=0 means do not cache.
      return true;
    }
  }

  const std::string& expires = resource.GetResponseHeader("Expires");
  int64 expires_value = 0;
  if (!expires.empty() &&
      !ParseTimeValuedHeader(expires.c_str(), &expires_value)) {
    // An invalid Expires header (e.g. Expires: 0) means do not cache.
    return true;
  }

  const std::string& pragma = resource.GetResponseHeader("Pragma");
  if (pragma.find("no-cache") != pragma.npos) {
    return true;
  }

  const std::string& vary = resource.GetResponseHeader("Vary");
  if (vary.find("*") != vary.npos) {
    return true;
  }

  return false;
}

bool HasExplicitFreshnessLifetime(const Resource& resource) {
  int64 freshness_lifetime = 0;
  return GetFreshnessLifetimeMillis(resource, &freshness_lifetime);
}

bool IsCacheableResourceStatusCode(int status_code) {
  switch (status_code) {
    // HTTP/1.1 RFC lists these response codes as heuristically
    // cacheable in the absence of explicit caching headers. The
    // primary cacheable status code is 200, but 203 and 206 are also
    // listed in the RFC.
    case 200:
    case 203:
    case 206:
      return true;

    // In addition, 304s are sent for cacheable resources. Though the
    // 304 response itself is not cacheable, the underlying resource
    // is, and that's what we care about.
    case 304:
      return true;

    default:
      return false;
  }
}

bool IsLikelyStaticResourceType(pagespeed::ResourceType type) {
  switch (type) {
    case IMAGE:
    case CSS:
    case FLASH:
    case JS:
      // These resources are almost always cacheable.
      return true;

    case REDIRECT:
      // Redirects can be cacheable.
      return true;

    case OTHER:
      // If other, some content types (e.g. flash, video) are static
      // while others are not. Be conservative for now and assume
      // non-cacheable.
      //
      // TODO: perhaps if there's a common mime prefix for the
      // cacheable types (e.g. application/), check to see that the
      // prefix is present.
      return false;

    default:
      return false;
  }
}

bool ParseTimeValuedHeader(const char* time_str, int64 *out_epoch_millis) {
  if (time_str == NULL || *time_str == '\0') {
    *out_epoch_millis = 0;
    return false;
  }
  PRTime result_time = 0;
  PRStatus result = PR_ParseTimeString(time_str, PR_FALSE, &result_time);
  if (PR_SUCCESS != result) {
    return false;
  }

  *out_epoch_millis = result_time / 1000;
  return true;
}

bool GetFreshnessLifetimeMillis(const Resource& resource,
                                int64 *out_freshness_lifetime_millis) {
  // Initialize the output param to the default value. We do this in
  // case clients use the out value without checking the return value
  // of the function.
  *out_freshness_lifetime_millis = 0;

  if (HasExplicitNoCacheDirective(resource)) {
    // If there's an explicit no cache directive then the resource is
    // never fresh.
    return true;
  }

  // First, look for Cache-Control: max-age. The HTTP/1.1 RFC
  // indicates that CC: max-age takes precedence to Expires.
  const std::string& cache_control =
      resource.GetResponseHeader("Cache-Control");
  DirectiveMap cache_directives;
  if (!GetHeaderDirectives(cache_control, &cache_directives)) {
    LOG(ERROR) << "Failed to parse cache control directives for "
               << resource.GetRequestUrl();
  } else {
    DirectiveMap::const_iterator it = cache_directives.find("max-age");
    if (it != cache_directives.end()) {
      int64 max_age_value = 0;
      if (StringToInt64(it->second, &max_age_value)) {
        *out_freshness_lifetime_millis = max_age_value * 1000;
        return true;
      }
    }
  }

  // Next look for Expires.
  const std::string& expires = resource.GetResponseHeader("Expires");
  if (expires.empty()) {
    // If there's no expires header and we previously determined there
    // was no Cache-Control: max-age, then the resource doesn't have
    // an explicit freshness lifetime.
    return false;
  }

  // We've determined that there is an Expires header. Thus, the
  // resource has a freshness lifetime. Even if the Expires header
  // doesn't contain a valid date, it should be considered stale. From
  // HTTP/1.1 RFC 14.21: "HTTP/1.1 clients and caches MUST treat other
  // invalid date formats, especially including the value "0", as in
  // the past (i.e., "already expired")."

  const std::string& date = resource.GetResponseHeader("Date");
  int64 date_value = 0;
  if (date.empty() || !ParseTimeValuedHeader(date.c_str(), &date_value)) {
    LOG(ERROR) << "Missing or invalid date header: '" << date << "'. "
               << "Assuming resource " << resource.GetRequestUrl()
               << " is not cacheable.";
    // We have an Expires header, but no Date header to reference
    // from. Thus we assume that the resource is heuristically
    // cacheable, but not explicitly cacheable.
    return false;
  }

  int64 expires_value = 0;
  if (!ParseTimeValuedHeader(expires.c_str(), &expires_value)) {
    // If we can't parse the Expires header, then treat the resource as
    // stale.
    return true;
  }

  int64 freshness_lifetime_millis = expires_value - date_value;
  if (freshness_lifetime_millis < 0) {
    freshness_lifetime_millis = 0;
  }
  *out_freshness_lifetime_millis = freshness_lifetime_millis;
  return true;
}

bool IsCacheableResource(const Resource& resource) {
  int64 freshness_lifetime = 0;
  if (GetFreshnessLifetimeMillis(resource, &freshness_lifetime)) {
    if (freshness_lifetime <= 0) {
      // The resource is explicitly not fresh, so we don't consider it
      // to be a static resource.
      return false;
    }

    // If there's an explicit freshness lifetime and it's greater than
    // zero, then the resource is cacheable.
    return true;
  }

  // If we've made it this far, we've got a resource that doesn't have
  // explicit caching headers. At this point we use heuristics
  // specified in the HTTP RFC and implemented in many
  // browsers/proxies to determine if this resource is typically
  // cached.
  return IsHeuristicallyCacheable(resource);
}

bool IsLikelyStaticResource(const Resource& resource) {
  if (!IsCacheableResourceStatusCode(resource.GetResponseStatusCode())) {
    return false;
  }

  if (!IsCacheableResource(resource)) {
    return false;
  }

  if (!IsLikelyStaticResourceType(resource.GetResourceType())) {
    // Certain types of resources (e.g. JS, CSS, images) are typically
    // static. If the resource isn't one of these types, assume it's
    // not static.
    return false;
  }

  // The resource passed all of the checks, so it appears to be
  // static.
  return true;
}

}  // namespace resource_util

}  // namespace pagespeed
