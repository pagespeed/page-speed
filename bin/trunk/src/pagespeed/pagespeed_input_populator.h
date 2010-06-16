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

#ifndef PAGESPEED_PAGESPEED_INPUT_POPULATOR_H_
#define PAGESPEED_PAGESPEED_INPUT_POPULATOR_H_

class URLRequestJob;

namespace pagespeed {

class JobTracker;
class PagespeedInput;

// PagespeedInputPopulator attaches itself to the Chromium network
// stack and records all of Chromium's in-flight network requests in a
// PagespeedInput instance.
class PagespeedInputPopulator {
 public:
  PagespeedInputPopulator();
  ~PagespeedInputPopulator();

  // Attaches to the network stack and begins recording.
  void Attach();

  // Detaches from the network stack and returns the set of recorded resources.
  pagespeed::PagespeedInput* Detach();

 private:
  JobTracker* tracker_;
};

}  // namespace pagespeed

#endif  // PAGESPEED_PAGESPEED_INPUT_POPULATOR_H_
