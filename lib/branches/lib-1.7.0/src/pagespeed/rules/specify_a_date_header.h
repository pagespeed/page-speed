// Copyright 2010 Google Inc. All Rights Reserved.
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

#ifndef PAGESPEED_RULES_SPECIFY_A_DATE_HEADER_H_
#define PAGESPEED_RULES_SPECIFY_A_DATE_HEADER_H_

#include "base/basictypes.h"
#include "pagespeed/core/rule.h"

namespace pagespeed {

class PagespeedInput;
class Results;

namespace rules {

/**
 * Checks for requests that are missing a date header. The Date header
 * is a required HTTP/1.1 response header, and according to the
 * HTTP/1.1 RFC, it is necessary in order to compute the freshness
 * lifetime of a resource, which is needed in order to determine if a
 * cached resource is valid. Currently, the major browsers will use
 * the resource response time if a Date header is not provided, but
 * previous releases of some browsers have failed to optimally cache
 * content that is missing a date header. In addition, all versions of
 * the Squid HTTP proxy up to 3.0.STABLE1 will not cache resources
 * that are missing a date header, even when a future Cache-Control
 * max-age is specified. This behavior is consistent with the HTTP
 * RFC. The short summary is: always specify a valid date header.
 */
class SpecifyADateHeader : public Rule {
 public:
  SpecifyADateHeader();

  // Rule interface.
  virtual const char* name() const;
  virtual const char* header() const;
  virtual const char* documentation_url() const;
  virtual bool AppendResults(const PagespeedInput& input,
                             ResultProvider* provider);
  virtual void FormatResults(const ResultVector& results, Formatter* formatter);

  DISALLOW_COPY_AND_ASSIGN(SpecifyADateHeader);
};

}  // namespace rules

}  // namespace pagespeed

#endif  // PAGESPEED_RULES_SPECIFY_A_DATE_HEADER_H_
