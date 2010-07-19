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

#include "pagespeed/pagespeed_input_populator.h"

#include <map>
#include <string>

#include "base/logging.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"
#include "net/url_request/url_request_job_tracker.h"
#include "pagespeed/http_content_decoder.h"
#include "third_party/libpagespeed/src/pagespeed/core/pagespeed_input.h"

namespace {

const char* GetOriginalUrlForJob(URLRequestJob* job) {
  if (job && job->request()) {
    std::string url = job->request()->original_url().possibly_invalid_spec();
    size_t hash_location = url.find('#');
    if (hash_location != url.npos) {
      url.erase(hash_location);
    }
    return url.c_str();
  }
  return NULL;
}

net::HttpResponseHeaders* GetResponseHeadersForJob(URLRequestJob* job) {
  if (job == NULL || job->request() == NULL) {
    return NULL;
  }

  std::string newline_terminated_headers;
  job->request()->GetAllResponseHeaders(&newline_terminated_headers);
  if (newline_terminated_headers.empty()) {
    return NULL;
  }

  std::string raw_headers = net::HttpUtil::AssembleRawHeaders(
      newline_terminated_headers.c_str(), newline_terminated_headers.size());
  return new net::HttpResponseHeaders(raw_headers);
}

}  // namespace

namespace pagespeed {

class JobTracker : public URLRequestJobTracker::JobObserver {
 public:
  virtual ~JobTracker() {
    if (!in_flight_responses_.empty()) {
      LOG(ERROR) << in_flight_responses_.size() << " in-flight responses.";
    }
  }

  void Init() {
    in_flight_responses_.clear();
    input_.reset(new pagespeed::PagespeedInput());
  }

  pagespeed::PagespeedInput* StealInput() {
    return input_.release();
  }

  virtual void OnJobAdded(URLRequestJob* job) {
    in_flight_responses_[job] = "";
  }

  // The remaining methods implement URLRequestJobTracker::JobObserver
  virtual void OnJobRemoved(URLRequestJob* job) {
    std::map<const URLRequestJob*, std::string>::iterator it =
        in_flight_responses_.find(job);
    in_flight_responses_.erase(it);
  }

  virtual void OnJobDone(URLRequestJob* job,
                         const URLRequestStatus& status) {
    const char* url = GetOriginalUrlForJob(job);
    if (url == NULL) {
      // Invalid job. Ignore it.
      return;
    }

    scoped_refptr<net::HttpResponseHeaders> headers =
        GetResponseHeadersForJob(job);
    if (headers == NULL) {
      // We receive some bogus URLRequestJobs that don't have any
      // headers, so we must check for that case and ignore those
      // responses.
      return;
    }

    scoped_ptr<pagespeed::Resource> resource(new pagespeed::Resource());
    resource->SetRequestUrl(url);
    resource->SetRequestMethod(job->request()->method());
    if (job->request()->has_upload()) {
      // TODO(bmcquade): UploadData can be either TYPE_FILE or
      // TYPE_BYTES. We should write code to handle both types. Since
      // no rules look at POST bodies at the moment, we can defer this
      // work.
    }

    // TODO(bmcquade): Extend URLRequest to expose the full set of
    // request headers, and populate the Resource's request headers
    // here.

    std::string name;
    std::string value;
    void* iter = NULL;
    while (headers->EnumerateHeaderLines(&iter, &name, &value)) {
      resource->AddResponseHeader(name, value);
    }
    resource->SetResponseStatusCode(headers->response_code());

    HttpContentDecoder decoder(job, in_flight_responses_[job]);
    if (decoder.NeedsDecoding()) {
      std::string decoded_body;
      if (decoder.Decode(&decoded_body)) {
        resource->SetResponseBody(decoded_body);
      }
    } else {
    resource->SetResponseBody(in_flight_responses_[job]);
    }
    input_->AddResource(resource.release());
  }

  virtual void OnJobRedirect(URLRequestJob* job, const GURL& location,
                             int status_code) {
  }

  virtual void OnBytesRead(URLRequestJob* job,
                           // const char* buf,
                           int byte_count) {
    /*
      TODO(bmcquade): add this when OnBytesRead callback includes the
      data buffer.

    if (buf) {
      in_flight_responses_[job].append(buf, byte_count);
    }
    */
  }

 private:
  // Buffers to store the bodies of in-flight HTTP responses.
  std::map<const URLRequestJob*, std::string> in_flight_responses_;

  // The actual PagespeedInput structure which accumulates all of the
  // final data.
  scoped_ptr<pagespeed::PagespeedInput> input_;
};

PagespeedInputPopulator::PagespeedInputPopulator() : tracker_(NULL) {
}

PagespeedInputPopulator::~PagespeedInputPopulator() {
  Detach();
}

void PagespeedInputPopulator::Attach() {
  Detach();
  tracker_ = new JobTracker();
  tracker_->Init();
  g_url_request_job_tracker.AddObserver(tracker_);
}

pagespeed::PagespeedInput* PagespeedInputPopulator::Detach() {
  pagespeed::PagespeedInput* input = NULL;
  if (tracker_ != NULL) {
    g_url_request_job_tracker.RemoveObserver(tracker_);
    input = tracker_->StealInput();
    delete tracker_;
    tracker_ = NULL;
  }
  return input;
}

}  // namespace pagespeed
