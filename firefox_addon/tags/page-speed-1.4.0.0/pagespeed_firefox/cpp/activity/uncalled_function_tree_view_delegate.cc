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

#include "uncalled_function_tree_view_delegate.h"

#include <algorithm>

#include "call_graph_metadata.h"
#include "call_graph_profile.h"
#include "call_graph_util.h"
#include "find_first_invocations_visitor.h"
#include "profile.pb.h"
#include "check.h"

namespace activity {

UncalledFunctionTreeViewDelegate::UncalledFunctionTreeViewDelegate(
    const CallGraphProfile &profile)
    : profile_(profile) {
}

UncalledFunctionTreeViewDelegate::~UncalledFunctionTreeViewDelegate() {}

void UncalledFunctionTreeViewDelegate::Initialize(
    const FindFirstInvocationsVisitor &visitor) {
  uncalled_function_tags_.clear();
  PopulateUncalledVector(visitor);
  ::std::sort(uncalled_function_tags_.begin(), uncalled_function_tags_.end());
}

int32_t UncalledFunctionTreeViewDelegate::GetRowCount() {
  return uncalled_function_tags_.size();
}

bool UncalledFunctionTreeViewDelegate::GetCellText(
    int32_t row_index, int32_t column, std::string *retval) {
  if (row_index < 0 || row_index >= GetRowCount()) {
    return false;
  }

  if (retval == NULL) {
    return false;
  }

  const ColumnId column_id = static_cast<ColumnId>(column);
  const int32_t function_tag = uncalled_function_tags_[row_index];
  CallGraphMetadata::MetadataMap::const_iterator it =
      profile_.metadata()->map()->find(function_tag);
  GCHECK(it != profile_.metadata()->map()->end());
  const FunctionMetadata &function_metadata = *it->second;

  switch (column_id) {
    case INSTANTIATION_TIME:
      GCHECK(function_metadata.has_function_instantiation_time_usec());
      util::FormatTime(
          function_metadata.function_instantiation_time_usec(), retval);
      return true;

    case FUNCTION_NAME:
      *retval = function_metadata.function_name();
      return true;

    case FILE_NAME:
      *retval = function_metadata.file_name();
      return true;

    case FUNCTION_SOURCE:
      *retval = function_metadata.function_source_utf8();
      return true;

    default:
      return false;
  }
}

void UncalledFunctionTreeViewDelegate::PopulateUncalledVector(
    const FindFirstInvocationsVisitor &visitor) {
  const FindFirstInvocationsVisitor::InvokedFunctionTags &invoked_tags =
      *visitor.invoked_tags();
  for (CallGraphMetadata::MetadataMap::const_iterator
           it = profile_.metadata()->map()->begin(),
           end = profile_.metadata()->map()->end();
       it != end;
       ++it) {
    const FunctionMetadata &function_metadata = *it->second;
    if (function_metadata.function_name().empty()) {
      // Entries with empty names are actually top-level script
      // blocks. Skip them.
      continue;
    }
    if (!profile_.ShouldIncludeInProfile(
            function_metadata.file_name().c_str())) {
      continue;
    }

    const int32_t function_tag = function_metadata.function_tag();
    if (invoked_tags.find(function_tag) == invoked_tags.end()) {
      uncalled_function_tags_.push_back(function_tag);
    }
  }
}

}  // namespace activity
