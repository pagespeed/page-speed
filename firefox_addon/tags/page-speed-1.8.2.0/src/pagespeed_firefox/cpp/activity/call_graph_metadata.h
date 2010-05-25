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
// CallGraphMetadata holds information (e.g. function name, source,
// etc) about the functions recorded in the associated CallGraph
// structure. The Firefox JavaScript debugger identifies functions by
// their "tag", an int identifier. CallGraphMetadata maps from a tag
// to the associated metadata. CallGraphMetadata is not
// thread-safe. If you need to access a CallGraphMetadata instance
// from multiple threads, create a read-only snapshot of the
// CallGraphMetadata using CreateSnapshot.

#ifndef CALL_GRAPH_METADATA_H_
#define CALL_GRAPH_METADATA_H_

#include "base/basictypes.h"
#include "portable_hash_map.h"

namespace activity {

class FunctionMetadata;
class Profile;

// See comment at top of file for a complete description
class CallGraphMetadata {
 public:
  typedef portable_hash_map<int32, const FunctionMetadata*> MetadataMap;

  explicit CallGraphMetadata(Profile *profile);

  // Do we have an entry for the function with the given tag?
  bool HasEntry(int32 tag) const;

  // Add an entry for the function with the associated
  // identifier. tag, file_name, function_name, and
  // function_source_utf8 are required parameters.
  // function_instantiation_time_usec is optional; if the
  // instantiation time of the function is unknown, you should pass -1
  // for this value.
  bool AddEntry(int32 tag,
                const char *file_name,
                const char *function_name,
                const char *function_source_utf8,
                int64 function_instantiation_time_usec);

  const MetadataMap *map() const { return &metadata_map_; }

  // Create a read-only thread-safe view of this CallGraphMetadata.
  // The returned object is backed by the same data store as this
  // object, so the caller must ensure that the read-only view is
  // destroyed before this object is destroyed. Ownership of the
  // returned object is transferred to the caller.
  CallGraphMetadata *CreateSnapshot() const;

 private:
  // Map from tag to the associated metadata.
  MetadataMap metadata_map_;

  Profile *const profile_;

  DISALLOW_COPY_AND_ASSIGN(CallGraphMetadata);
};

}  // namespace activity


#endif  // CALL_GRAPH_METADATA_H_
