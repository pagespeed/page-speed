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

#include "pagespeed/proto/proto_resource_utils.h"

#include <map>
#include <string>
#include <vector>

#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/proto/pagespeed_input.pb.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace proto {

void PopulateResource(const ProtoResource& input, Resource* output) {
  output->SetRequestUrl(input.request_url());
  output->SetRequestMethod(input.request_method());
  output->SetRequestBody(input.request_body());
  output->SetResponseProtocol(input.response_protocol());
  output->SetResponseStatusCode(input.response_status_code());
  output->SetResponseBody(input.response_body());

  typedef ::google::protobuf::RepeatedPtrField<ProtoResource::Header>
      HeaderList;

  const HeaderList& request_headers = input.request_headers();
  for (HeaderList::const_iterator iter = request_headers.begin(),
           end = request_headers.end();
       iter != end;
       ++iter) {
    output->AddRequestHeader(iter->key(), iter->value());
  }

  const HeaderList& response_headers = input.response_headers();
  for (HeaderList::const_iterator iter = response_headers.begin(),
           end = response_headers.end();
       iter != end;
       ++iter) {
    output->AddResponseHeader(iter->key(), iter->value());
  }
}

void PopulatePagespeedInput(const ProtoInput& proto_input,
                            PagespeedInput* pagespeed_input) {
  typedef ::google::protobuf::RepeatedPtrField<ProtoResource>
      ResourceList;

  const ResourceList& serialized_resources = proto_input.resources();
  for (ResourceList::const_iterator iter = serialized_resources.begin(),
           end = serialized_resources.end();
       iter != end;
       ++iter) {
    Resource* resource = new Resource;
    PopulateResource(*iter, resource);
    pagespeed_input->AddResource(resource);
  }
}

void PopulateProtoResource(const Resource& input, ProtoResource* output) {
  output->set_request_url(input.GetRequestUrl());
  output->set_request_method(input.GetRequestMethod());
  output->set_request_body(input.GetRequestBody());
  output->set_response_protocol(input.GetResponseProtocolString());
  output->set_response_status_code(input.GetResponseStatusCode());
  output->set_response_body(input.GetResponseBody());

  const Resource::HeaderMap& request_headers = *input.GetRequestHeaders();
  for (Resource::HeaderMap::const_iterator iter = request_headers.begin(),
           end = request_headers.end();
       iter != end;
       ++iter) {
    ProtoResource::Header* header = output->add_request_headers();
    header->set_key(iter->first);
    header->set_value(iter->second);
  }

  const Resource::HeaderMap& response_headers = *input.GetResponseHeaders();
  for (Resource::HeaderMap::const_iterator iter = response_headers.begin(),
           end = response_headers.end();
       iter != end;
       ++iter) {
    ProtoResource::Header* header = output->add_response_headers();
    header->set_key(iter->first);
    header->set_value(iter->second);
  }
}

void PopulateProtoInput(const PagespeedInput& input, ProtoInput* proto_input) {
  for (int idx = 0; idx < input.num_resources(); ++idx) {
    PopulateProtoResource(input.GetResource(idx),
                          proto_input->add_resources());
  }
}

}  // namespace proto

}  // namespace pagespeed
