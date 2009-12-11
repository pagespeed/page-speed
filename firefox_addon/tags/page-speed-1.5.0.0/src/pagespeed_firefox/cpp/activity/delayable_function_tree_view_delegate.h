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
// DelayableFunctionTreeViewDelegate implements
// BasicTreeViewDelegateInterface and provides a list of functions
// ordered by the difference between their instantiation time and
// their first invocation time (the amount of instantiation delay
// possible for the function).

#ifndef DELAYABLE_FUNCTION_TREE_VIEW_DELEGATE_H_
#define DELAYABLE_FUNCTION_TREE_VIEW_DELEGATE_H_

#include <string>
#include <utility>  // for pair<>
#include <vector>

#include "base/basictypes.h"
#include "basic_tree_view_delegate_interface.h"

namespace activity {

class CallGraphProfile;
class FindFirstInvocationsVisitor;
class FunctionMetadata;

class DelayableFunctionTreeViewDelegate
    : public BasicTreeViewDelegateInterface {
 public:
  explicit DelayableFunctionTreeViewDelegate(const CallGraphProfile &profile);
  virtual ~DelayableFunctionTreeViewDelegate();

  /**
   * Initialize this DelayableFunctionTreeViewDelegate based on the
   * contents of the FindFirstInvocationsVisitor.
   */
  void Initialize(const FindFirstInvocationsVisitor &visitor);

  // Implement BasicTreeViewDelegateInterface.
  virtual int32 GetRowCount();
  virtual bool GetCellText(int32 row, int32 column, std::string *retval);

  // The columns in the tree view. Must be declared in the same order
  // as the XUL tree widget.
  enum ColumnId {
    DELAY = 0,
    INSTANTIATION_TIME,
    FIRST_CALL,
    FUNCTION_NAME,
    FUNCTION_SOURCE,
    FILE_NAME,
    LAST_COLUMN_ID = FILE_NAME
  };

 private:
  typedef std::pair<int64, int32> TimeTagPair;

  // Populate the given vector with pairs that contain the difference
  // between the function instantiation time and the first function
  // call time, and the associated function tag.
  void PopulateInstantiationDelayVector(
      const FindFirstInvocationsVisitor &visitor);

  // Get the FunctionMetadata for the given tag, or NULL if there is
  // no FunctionMetadata for the given tag.
  const FunctionMetadata *GetMetadataOrNull(int32 function_tag) const;

  std::vector<TimeTagPair> tags_in_delay_order_;
  const CallGraphProfile &profile_;

  DISALLOW_COPY_AND_ASSIGN(DelayableFunctionTreeViewDelegate);
};

}  // namespace activity

#endif  // DELAYABLE_FUNCTION_TREE_VIEW_DELEGATE_H_
