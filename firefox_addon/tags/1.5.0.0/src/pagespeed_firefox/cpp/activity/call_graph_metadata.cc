/**
 * Copyright 2008-2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: Bryan McQuade
//
// CallGraphMetadata implementation.  See call_graph_metadata.h for details.

#include "call_graph_metadata.h"

#include <string>

#include "profile.pb.h"
#include "check.h"

namespace activity {

CallGraphMetadata::CallGraphMetadata(Profile *profile)
    : profile_(profile) {
}

CallGraphMetadata *CallGraphMetadata::CreateSnapshot() const {
  CallGraphMetadata *metadata = new CallGraphMetadata(profile_);
  metadata->metadata_map_ = metadata_map_;
  return metadata;
}

bool CallGraphMetadata::HasEntry(int32 tag) const {
  return metadata_map_.count(tag) != 0;
}

void CallGraphMetadata::AddEntry(int32 tag,
                                 const char *file_name,
                                 const char *function_name,
                                 const char *function_source_utf8,
                                 int64 function_instantiation_time_usec) {
  GCHECK(!HasEntry(tag));
  FunctionMetadata *entry = profile_->add_function_metadata();

  // Clean up inputs.
  if (file_name == NULL) file_name = "";
  if (function_name == NULL) function_name = "";
  if (function_source_utf8 == NULL) function_source_utf8 = "";

  // Remove the '#foo' part of the URL, if present, since the hash is
  // not part of the URL sent to the server.
  const char *url_hash_index = strchr(file_name, '#');
  if (url_hash_index != NULL) {
    int length = url_hash_index - file_name;
    std::string file_name_str(file_name, length);
    entry->set_file_name(file_name_str);
  } else {
    entry->set_file_name(file_name);
  }

  entry->set_function_tag(tag);
  entry->set_function_name(function_name);
  entry->set_function_source_utf8(function_source_utf8);

  if (function_instantiation_time_usec >= 0) {
    // If the function's instantiation time is specified, record it.
    entry->set_function_instantiation_time_usec(
        function_instantiation_time_usec);
  }

  metadata_map_[tag] = entry;
}

}  // namespace activity
