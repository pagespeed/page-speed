// Copyright 2011 Google Inc. All Rights Reserved.
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
//
// Author: bmcquade@google.com (Bryan McQuade)
// Author: sligocki@google.com (Shawn Ligocki)

#include "pagespeed/core/resource_cache_computer.h"

#include "base/logging.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"

namespace pagespeed {

ResourceCacheComputer::~ResourceCacheComputer() {
}

// Lazy getters

bool ResourceCacheComputer::IsCacheable() {
  if (!is_cacheable_.has_value()) {
    is_cacheable_.set_value(ComputeIsCacheable());
  }
  return is_cacheable_.value();
}

bool ResourceCacheComputer::IsProxyCacheable() {
  if (!is_proxy_cacheable_.has_value()) {
    is_proxy_cacheable_.set_value(ComputeIsProxyCacheable());
  }
  return is_proxy_cacheable_.value();
}

bool ResourceCacheComputer::IsHeuristicallyCacheable() {
  if (!is_heuristically_cacheable_.has_value()) {
    is_heuristically_cacheable_.set_value(ComputeIsHeuristicallyCacheable());
  }
  return is_heuristically_cacheable_.value();
}

bool ResourceCacheComputer::GetFreshnessLifetimeMillis(
    int64* out_freshness_lifetime_millis) {
  DCHECK_EQ(is_explicitly_cacheable_.has_value(),
            freshness_lifetime_millis_.has_value());
  if (!is_explicitly_cacheable_.has_value() ||
      !freshness_lifetime_millis_.has_value()) {
    int64 freshness_lifetime_millis = 0;
    is_explicitly_cacheable_.set_value(
        ComputeFreshnessLifetimeMillis(&freshness_lifetime_millis));
    freshness_lifetime_millis_.set_value(freshness_lifetime_millis);
  }
  *out_freshness_lifetime_millis = freshness_lifetime_millis_.value();
  return is_explicitly_cacheable_.value();
}

bool ResourceCacheComputer::HasExplicitNoCacheDirective() {
  if (!has_explicit_no_cache_directive_.has_value()) {
    has_explicit_no_cache_directive_.set_value(
        ComputeHasExplicitNoCacheDirective());
  }
  return has_explicit_no_cache_directive_.value();
}

// Simple wrapper functions

bool ResourceCacheComputer::IsExplicitlyCacheable() {
  int64 freshness_lifetime = 0;
  return (GetFreshnessLifetimeMillis(&freshness_lifetime) &&
          (freshness_lifetime > 0));
}

bool ResourceCacheComputer::HasExplicitFreshnessLifetime() {
  int64 freshness_lifetime = 0;
  return GetFreshnessLifetimeMillis(&freshness_lifetime);
}

bool ResourceCacheComputer::IsLikelyStaticResourceType() {
  return resource_util::IsLikelyStaticResourceType(
      resource_->GetResourceType());
}

bool ResourceCacheComputer::IsCacheableResourceStatusCode() {
  return resource_util::IsCacheableResourceStatusCode(
      resource_->GetResponseStatusCode());
}

// Actual compute logic

bool ResourceCacheComputer::ComputeIsCacheable() {
  int64 freshness_lifetime = 0;
  if (GetFreshnessLifetimeMillis(&freshness_lifetime)) {
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
  return IsHeuristicallyCacheable();
}

bool ResourceCacheComputer::ComputeIsProxyCacheable() {
  if (!IsCacheable()) {
    return false;
  }

  resource_util::DirectiveMap directive_map;
  if (!resource_util::GetHeaderDirectives(
          resource_->GetResponseHeader("Cache-Control"), &directive_map)) {
    return false;
  }

  if (directive_map.find("private") != directive_map.end()) {
    return false;
  }

  return true;
}

bool ResourceCacheComputer::ComputeIsHeuristicallyCacheable() {
  if (HasExplicitFreshnessLifetime()) {
    // If the response has an explicit freshness lifetime then it's
    // not heuristically cacheable. This method only expects to be
    // called if the resource does *not* have an explicit freshness
    // lifetime.
    LOG(DFATAL) << "IsHeuristicallyCacheable received a resource with "
                << "explicit freshness lifetime.";
    return false;
  }

  resource_util::DirectiveMap cache_directives;
  if (!resource_util::GetHeaderDirectives(
          resource_->GetResponseHeader("Cache-Control"), &cache_directives)) {
    LOG(INFO) << "Failed to parse cache control directives for "
              << resource_->GetRequestUrl();
    return false;
  }

  if (cache_directives.find("must-revalidate") != cache_directives.end()) {
    // must-revalidate indicates that a non-fresh response should not
    // be used in response to requests without validating at the
    // origin. Such a resource is not heuristically cacheable.
    return false;
  }

  const std::string& url = resource_->GetRequestUrl();
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

  if (!IsCacheableResourceStatusCode()) {
    return false;
  }

  return true;
}

bool ResourceCacheComputer::ComputeFreshnessLifetimeMillis(
    int64* out_freshness_lifetime_millis) {
  // Initialize the output param to the default value. We do this in
  // case clients use the out value without checking the return value
  // of the function.
  *out_freshness_lifetime_millis = 0;

  if (HasExplicitNoCacheDirective()) {
    // If there's an explicit no cache directive then the resource is
    // never fresh.
    return true;
  }

  // First, look for Cache-Control: max-age. The HTTP/1.1 RFC
  // indicates that CC: max-age takes precedence to Expires.
  const std::string& cache_control =
      resource_->GetResponseHeader("Cache-Control");
  resource_util::DirectiveMap cache_directives;
  if (!resource_util::GetHeaderDirectives(cache_control, &cache_directives)) {
    LOG(INFO) << "Failed to parse cache control directives for "
              << resource_->GetRequestUrl();
  } else {
    resource_util::DirectiveMap::const_iterator it =
        cache_directives.find("max-age");
    if (it != cache_directives.end()) {
      int64 max_age_value = 0;
      if (base::StringToInt64(it->second, &max_age_value)) {
        *out_freshness_lifetime_millis = max_age_value * 1000;
        return true;
      }
    }
  }

  // Next look for Expires.
  const std::string& expires = resource_->GetResponseHeader("Expires");
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

  const std::string& date = resource_->GetResponseHeader("Date");
  int64 date_value = 0;
  if (date.empty() ||
      !resource_util::ParseTimeValuedHeader(date.c_str(), &date_value)) {
    LOG(INFO) << "Missing or invalid date header: '" << date << "'. "
              << "Assuming resource " << resource_->GetRequestUrl()
              << " is not cacheable.";
    // We have an Expires header, but no Date header to reference
    // from. Thus we assume that the resource is heuristically
    // cacheable, but not explicitly cacheable.
    return false;
  }

  int64 expires_value = 0;
  if (!resource_util::ParseTimeValuedHeader(expires.c_str(), &expires_value)) {
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

bool ResourceCacheComputer::ComputeHasExplicitNoCacheDirective() {
  resource_util::DirectiveMap cache_directives;
  if (!resource_util::GetHeaderDirectives(
          resource_->GetResponseHeader("Cache-Control"), &cache_directives)) {
    LOG(INFO) << "Failed to parse cache control directives for "
              << resource_->GetRequestUrl();
    return true;
  }

  if (cache_directives.find("no-cache") != cache_directives.end()) {
    return true;
  }
  if (cache_directives.find("no-store") != cache_directives.end()) {
    return true;
  }
  resource_util::DirectiveMap::const_iterator it =
      cache_directives.find("max-age");
  if (it != cache_directives.end()) {
    int64 max_age_value = 0;
    if (base::StringToInt64(it->second, &max_age_value) &&
        max_age_value == 0) {
      // Cache-Control: max-age=0 means do not cache.
      return true;
    }
  }

  const std::string& expires = resource_->GetResponseHeader("Expires");
  int64 expires_value = 0;
  if (!expires.empty() &&
      !resource_util::ParseTimeValuedHeader(expires.c_str(), &expires_value)) {
    // An invalid Expires header (e.g. Expires: 0) means do not cache.
    return true;
  }

  const std::string& pragma = resource_->GetResponseHeader("Pragma");
  if (pragma.find("no-cache") != pragma.npos) {
    return true;
  }

  const std::string& vary = resource_->GetResponseHeader("Vary");
  if (vary.find("*") != vary.npos) {
    return true;
  }

  return false;
}

template<class T> ResourceCacheComputer::Optional<T>::~Optional() {
}

}  // namespace pagespeed
