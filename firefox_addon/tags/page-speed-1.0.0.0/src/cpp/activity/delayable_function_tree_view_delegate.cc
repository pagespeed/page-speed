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

#include "delayable_function_tree_view_delegate.h"

#include <algorithm>

#include "call_graph_metadata.h"
#include "call_graph_profile.h"
#include "call_graph_util.h"
#include "find_first_invocations_visitor.h"
#include "profile.pb.h"
#include "check.h"

namespace activity {

DelayableFunctionTreeViewDelegate::DelayableFunctionTreeViewDelegate(
    const CallGraphProfile &profile)
    : profile_(profile) {
}

DelayableFunctionTreeViewDelegate::~DelayableFunctionTreeViewDelegate() {}

void DelayableFunctionTreeViewDelegate::Initialize(
    const FindFirstInvocationsVisitor &visitor) {
  tags_in_delay_order_.clear();
  PopulateInstantiationDelayVector(visitor);
  ::std::sort(tags_in_delay_order_.begin(), tags_in_delay_order_.end());
}

int32_t DelayableFunctionTreeViewDelegate::GetRowCount() {
  return tags_in_delay_order_.size();
}

bool DelayableFunctionTreeViewDelegate::GetCellText(
    int32_t row_index, int32_t column_index, std::string *retval) {
  // Reverse the order so entries are shown from most delayable to
  // least delayable.
  row_index = (GetRowCount() - row_index) - 1;

  if (row_index < 0 || row_index >= GetRowCount()) {
    return false;
  }

  if (retval == NULL) {
    return false;
  }

  const ColumnId column_id = static_cast<ColumnId>(column_index);
  const int32_t function_tag = tags_in_delay_order_[row_index].second;
  const int64_t delay_time_usec = tags_in_delay_order_[row_index].first;
  CallGraphMetadata::MetadataMap::const_iterator it =
      profile_.metadata()->map()->find(function_tag);
  GCHECK(it != profile_.metadata()->map()->end());
  const FunctionMetadata &function_metadata = *it->second;

  switch (column_id) {
    case DELAY:
      util::FormatTime(delay_time_usec, retval);
      return true;

    case INSTANTIATION_TIME:
      GCHECK(function_metadata.has_function_instantiation_time_usec());
      util::FormatTime(
          function_metadata.function_instantiation_time_usec(), retval);
      return true;

    case FIRST_CALL:
      GCHECK(function_metadata.has_function_instantiation_time_usec());
      util::FormatTime(
          function_metadata.function_instantiation_time_usec() +
          delay_time_usec,
          retval);
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

const FunctionMetadata *DelayableFunctionTreeViewDelegate::GetMetadataOrNull(
    int32_t function_tag) const {
  const CallGraphMetadata::MetadataMap::const_iterator it =
      profile_.metadata()->map()->find(function_tag);
  if (it == profile_.metadata()->map()->end()) {
    return NULL;
  }
  return it->second;
}

void DelayableFunctionTreeViewDelegate::PopulateInstantiationDelayVector(
    const FindFirstInvocationsVisitor &visitor) {
  const FindFirstInvocationsVisitor::FirstInvocations &invocations =
      *visitor.invocations();
  for (FindFirstInvocationsVisitor::FirstInvocations::const_iterator
           it = invocations.begin(), end = invocations.end();
       it != end;
       ++it) {
    const CallTree &call_tree = **it;
    const int32_t function_tag = call_tree.function_tag();
    const FunctionMetadata *function_metadata = GetMetadataOrNull(function_tag);
    if (function_metadata == NULL) {
      // No metadata for this function. Skip it.
      continue;
    }

    if (function_metadata->function_name().empty()) {
      // Entries with empty names are actually top-level script
      // blocks. Skip them.
      continue;
    }

    if (!function_metadata->has_function_instantiation_time_usec()) {
      // No instantiation time for this function. Skip it.
      continue;
    }

    if (!profile_.ShouldIncludeInProfile(
            function_metadata->file_name().c_str())) {
      continue;
    }

    const int64_t possible_instantiation_delay_usec =
        call_tree.entry_time_usec() -
        function_metadata->function_instantiation_time_usec();
    GCHECK_GE(possible_instantiation_delay_usec, 0);

    tags_in_delay_order_.push_back(
        std::make_pair(possible_instantiation_delay_usec, function_tag));
  }
}

}  // namespace activity
